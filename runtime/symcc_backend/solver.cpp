#include "solver.h"
#include <byteswap.h>
#include <chrono>
#include <fstream>
#include <set>
#include <sstream>

namespace symcc {

namespace {
void parseConstSym(ExprRef e, Kind& op, ExprRef& expr_sym,
                   ExprRef& expr_const) {
    for (int32_t i = 0; i < 2; i++) {
        expr_sym = e->getChild(i);
        expr_const = e->getChild(1 - i);
        if (!isConstant(expr_sym) && isConstant(expr_const)) {
            op = i == 0 ? e->kind() : swapKind(e->kind());
            return;
        }
    }
    UNREACHABLE();
}

void getCanonicalExpr(ExprRef e, ExprRef* canonical,
                      llvm::APInt* adjustment = NULL) {
    ExprRef first = NULL;
    ExprRef second = NULL;
    // e == Const + Sym --> canonical == Sym
    switch (e->kind()) {
    // TODO: handle Sub
    case Add:
        first = e->getFirstChild();
        second = e->getSecondChild();
        if (isConstant(first)) {
            *canonical = second;
            if (adjustment != NULL)
                *adjustment =
                    std::static_pointer_cast<ConstantExpr>(first)->value();
            return;
        case Sub:
            // C_0 - Sym
            first = e->getFirstChild();
            second = e->getSecondChild();
            // XXX: need to handle reference count
            if (isConstant(first)) {
                *canonical = g_expr_builder->createNeg(second);
                if (adjustment != NULL)
                    *adjustment =
                        std::static_pointer_cast<ConstantExpr>(first)->value();
                return;
            }
        }
    default:
        break;
    }
    if (adjustment != NULL)
        *adjustment = llvm::APInt(e->bits(), 0);
    *canonical = e;
}

} // namespace

Solver::Solver(const std::vector<uint8_t>& ibuf, const std::string out_dir,
               const std::string log_file, const std::string stats_file,
               const std::string bitmap, unsigned kSolverTimeout)
    : inputs_(ibuf), out_dir_(out_dir), log_file_(log_file),
      stats_file_(stats_file), context_(*g_z3_context),
      solver_(z3::solver(context_, "QF_BV")), trace_(bitmap),
      last_interested_(false), syncing_(false), last_pc_(0),
      dep_forest_(ibuf.size() + 1), num_generated_(0) {
    // Set timeout for solver
    z3::params p(context_);
    p.set(":timeout", kSolverTimeout);
    solver_.set(p);
}

void Solver::push() { solver_.push(); }

void Solver::reset() {
    solver_.reset();
    skipped_constraints = 0;
    added_constraints = 0;
    symbolic_variables = 0;
    concrete_variables = 0;
}

void Solver::pop() { solver_.pop(); }

void Solver::add(z3::expr expr) {
    if (!expr.is_const())
        solver_.add(expr.simplify());
}

z3::check_result Solver::check() {
    z3::check_result res;
    try {
        auto start = std::chrono::high_resolution_clock::now();
        res = solver_.check();
        auto end = std::chrono::high_resolution_clock::now();
        solver_check_time_ = end - start;
        std::cerr << "SMT :{ \"solving_time\" : " << solver_check_time_.count()
                  << " }\n";
    } catch (z3::exception& e) {
        // https://github.com/Z3Prover/z3/issues/419
        // timeout can cause exception
        res = z3::unknown;
    }
    return res;
}

bool Solver::checkAndSave(const std::string& postfix) {
    if (check() == z3::sat) {
        saveValues(postfix);
        saveStats();
        return true;
    } else {
        std::cerr << ">> UNSAT\n";
        return false;
    }
}

void Solver::addJcc(ExprRef e, bool taken, ADDRINT pc) {
    // Save the last instruction pointer for debugging
    last_pc_ = pc;
#if 1
    std::cerr << "======================================== SOLVER:ADDJCC "
                 "======================================\n";
    std::cerr << e->toString() << std::endl;
    std::cerr << "======================================== SOLVER:ADDJCC "
                 "======================================\n";
#endif

    // if e == Bool(true), then ignore
    if (e->kind() == Bool) {
        assert(!(castAs<BoolExpr>(e)->value() ^ taken));
        return;
    }

    assert(isRelational(e.get()));

    // check duplication before really solving something,
    // some can be handled by range based constraint solving
    bool is_interesting;
    // = true; // BABY_MODE - eveyrthin is interesting, but it is brute
    // forcing branches and cases grow too much...

    if (pc == 0) {
        // If addJcc() is called by special case, then rely on
        is_interesting = last_interested_;
    } else {
        is_interesting = isInterestingJcc(e, taken, pc);
    }

    if (is_interesting) {
        // @Info(alekum 30/10/2022)
        // Save PC as a part of saved stats?
        negatePath(e, taken);
    }
    addConstraint(e, taken, is_interesting);

#if 0
    std::cerr << "========================= DEP FOREST AFTER SOLVING PUSHED "
                 "CONSTRAINT ==================================\n";
    dep_forest_.dump(std::cerr);
    std::cerr << "========================= DEP FOREST AFTER SOLVING PUSHED "
                 "CONSTRAINT ==================================\n\n";
#endif
}

void Solver::addAddr(ExprRef e, ADDRINT addr) {
    llvm::APInt v(e->bits(), addr);
    addAddr(e, v);
}

void Solver::addAddr(ExprRef e, llvm::APInt addr) {
    if (e->isConcrete())
        return;

    if (last_interested_) {
        reset();
        // TODO: add optimize in z3
        syncConstraints(e);
        if (check() != z3::sat)
            return;
        z3::expr& z3_expr = e->toZ3Expr();

        // TODO: add unbound case
        z3::expr min_expr = getMinValue(z3_expr);
        z3::expr max_expr = getMaxValue(z3_expr);
        solveOne(z3_expr == min_expr);
        solveOne(z3_expr == max_expr);
    }

    addValue(e, addr);
}

void Solver::addValue(ExprRef e, ADDRINT val) {
    llvm::APInt v(e->bits(), val);
    addValue(e, v);
}

void Solver::addValue(ExprRef e, llvm::APInt val) {
    if (e->isConcrete())
        return;

#ifdef CONFIG_TRACE
    trace_addValue(e, val);
#endif

    ExprRef expr_val = g_expr_builder->createConstant(val, e->bits());
    ExprRef expr_concrete =
        g_expr_builder->createBinaryExpr(Equal, e, expr_val);

    addConstraint(expr_concrete, true, false);
}

void Solver::solveAll(ExprRef e, llvm::APInt val) {
    if (last_interested_) {
        std::string postfix = "";
        ExprRef expr_val = g_expr_builder->createConstant(val, e->bits());
        ExprRef expr_concrete =
            g_expr_builder->createBinaryExpr(Equal, e, expr_val);

        reset();
        syncConstraints(e);
        addToSolver(expr_concrete, false);

        if (check() != z3::sat) {
            // Optimistic solving
            reset();
            addToSolver(expr_concrete, false);
            postfix = "optimistic";
        }

        z3::expr z3_expr = e->toZ3Expr();
        while (true) {
            if (!checkAndSave(postfix))
                break;
            z3::expr value = getPossibleValue(z3_expr);
            add(value != z3_expr);
        }
    }
    addValue(e, val);
}

uint8_t Solver::getInput(ADDRINT index) {
    assert(index < inputs_.size());
    return inputs_[index];
}

// @Info(alekum): Find a way to do this more effeciently
// as copying bunch of data here and there is quite
// costly operations. Probably, returning just
// touched bytes in our case more than enough...
std::vector<uint8_t> Solver::getConcreteValues() {
    // TODO: change from real input
    z3::model m = solver_.get_model();
    unsigned num_constants = m.num_consts();
    std::vector<uint8_t> values = inputs_;
    for (unsigned i = 0; i < num_constants; i++) {
        z3::func_decl decl = m.get_const_decl(i);
        z3::expr e = m.get_const_interp(decl);
        z3::symbol name = decl.name();

        if (name.kind() == Z3_INT_SYMBOL) {
            int value = e.get_numeral_int();
            values[name.to_int()] = (uint8_t)value;
        }
    }
    return values;
}

void Solver::saveStats() noexcept {
    std::ostringstream csv;
    csv << num_generated_ - 1 << ',' << solver_check_time_.count() << ','
        << sync_constraints_time_.count() << ',' << skipped_constraints << ','
        << added_constraints << ',' << symbolic_variables << ','
        << concrete_variables << std::endl;
    std::ofstream stat_file(stats_file_,
                            std::ios_base::out | std::ios_base::app);
    if (stat_file.fail()) {
        perror("Unable to open a stat file\n");
        stat_file.close();
        return;
    }
    stat_file << csv.str();
}

void Solver::saveValues(const std::string& postfix) {
    z3::model m = solver_.get_model();
    unsigned num_constants = m.num_consts();
    std::vector<uint8_t> values = inputs_;
    for (unsigned i = 0; i < num_constants; i++) {
        z3::func_decl decl = m.get_const_decl(i);
        z3::expr e = m.get_const_interp(decl);
        z3::symbol name = decl.name();

        if (name.kind() == Z3_INT_SYMBOL) {
            int value = e.get_numeral_int();
            values[name.to_int()] = (uint8_t)value;
        }
    }

    // If no output directory is specified, then just print it out
    if (out_dir_.empty()) {
        printValues(values);
        return;
    }
    std::ostringstream fname;
    fname << out_dir_ << '/' << std::setw(6) << std::setfill('0')
          << num_generated_;
    // Add postfix to record where it is genereated
    if (!postfix.empty())
        fname << '-' << postfix;
    std::ofstream of(fname.str(), std::ios_base::out | std::ios_base::binary);
    std::cerr << "[INFO] New testcase: " << fname.str() << std::endl;
    if (of.fail()) {
        perror("Unable to open a file to write results\n");
        of.close();
        return;
    }

    std::copy(values.begin(), values.end(), std::ostreambuf_iterator<char>(of));

    of.close();
    num_generated_++;
}

void Solver::printValues(const std::vector<uint8_t>& values) {
    fprintf(stderr, "[INFO] Values: ");
    for (unsigned i = 0; i < values.size(); i++) {
        fprintf(stderr, "\\x%02X", values[i]);
    }
    fprintf(stderr, "\n");
}

z3::expr Solver::getPossibleValue(z3::expr& z3_expr) {
    z3::model m = solver_.get_model();
    return m.eval(z3_expr);
}

z3::expr Solver::getMinValue(z3::expr& z3_expr) {
    push();
    z3::expr value(context_);
    while (true) {
        if (checkAndSave()) {
            value = getPossibleValue(z3_expr);
            solver_.add(z3::ult(z3_expr, value));
        } else
            break;
    }
    pop();
    return value;
}

z3::expr Solver::getMaxValue(z3::expr& z3_expr) {
    push();
    z3::expr value(context_);
    while (true) {
        if (checkAndSave()) {
            value = getPossibleValue(z3_expr);
            solver_.add(z3::ugt(z3_expr, value));
        } else
            break;
    }
    pop();
    return value;
}

void Solver::addToSolver(ExprRef e, bool taken) {
    e->simplify();
    if (!taken)
        e = g_expr_builder->createLNot(e);
    add(e->toZ3Expr());
}

void Solver::syncConstraints(ExprRef e) {
    std::set<std::shared_ptr<DependencyTree<Expr>>> forest;
    DependencySet& symdeps = e->getDeps();
    // we cannot have more symbolic variables than symbolic deps
    // basically it is the same stuff in current implementation.
    symbolic_variables = symdeps.size();

    // @Info(alekum): We can get partially symbolic expression,
    // so that we have to be sure that we turn everything into fully
    // symbolic
    for (const auto it : symdeps) {
        auto dt = dep_forest_.find(it);
        forest.insert(dt);

        auto se = symcc::cachedReadExpressions[it];
        if (se->isConcrete())
            se->symbolize(); // @Info(alekum): can we make this call cheaper??
    }
    // condeps.erase(symdeps.begin(), symdeps.end()); // free(): double free
    // detected in tcache 2
    for (std::shared_ptr<DependencyTree<Expr>> tree : forest) {
        for (const auto it : tree->getDependencies()) {
            if (!symdeps.count(it)) {
                ++concrete_variables;
                symcc::cachedReadExpressions[it]->concretize();
            }
        }

        for (std::shared_ptr<Expr> node : tree->getNodes()) {
            // @Info(alekum): if there are no common dependencies between node
            // and
            // processed ExprRef it means node is going to be fully
            // concretized, and we don't have to add them to solver anyway
            std::cerr << "Processing ... " << node->toString() << std::endl;
            if (node->isConcrete()) {
                std::cerr << "\t\tSkipppp ^^^^^\n";
                ++skipped_constraints;
                continue;
            }
            std::cerr << "\t\t Taken, add to solverrr ^^^^^\n";

            if (isRelational(node.get())) {
                addToSolver(node, true);
                ++added_constraints;
            } else {
                // Process range-based constraints
                bool valid = false;
                for (int32_t i = 0; i < 2; i++) {
                    // @Cleanup(alekum): Check if we could get rid of it
                    ExprRef expr_range = getRangeConstraint(node, i);
                    if (expr_range != NULL) {
                        addToSolver(expr_range, true);
                        ++added_constraints;
                        valid = true;
                    }
                }

                // One of range expressions should be non-NULL
                if (!valid)
                    LOG_INFO(std::string(__func__) +
                             ": Incorrect constraints are inserted\n");
            }
        }
    }
}

void Solver::addConstraint(ExprRef e, bool taken, bool is_interesting) {
    if (auto NE = castAs<LNotExpr>(e)) {
        addConstraint(NE->expr(), !taken, is_interesting);
        return;
    }
    if (!addRangeConstraint(e, taken))
        addNormalConstraint(e, taken);
}

void Solver::addConstraint(ExprRef e) {
    // If e is true, then just skip
    if (e->kind() == Bool) {
        SYMCC_ASSERT(castAs<BoolExpr>(e)->value());
        return;
    }
    dep_forest_.addNode(e);
}

bool Solver::addRangeConstraint(ExprRef e, bool taken) {
    if (!isConstSym(e))
        return false;

    Kind kind = Invalid;
    ExprRef expr_sym, expr_const;
    parseConstSym(e, kind, expr_sym, expr_const);
    ExprRef canonical = NULL;
    llvm::APInt adjustment;
    getCanonicalExpr(expr_sym, &canonical, &adjustment);
    llvm::APInt value =
        std::static_pointer_cast<ConstantExpr>(expr_const)->value();

    if (!taken)
        kind = negateKind(kind);

    canonical->addConstraint(kind, value, adjustment);
    addConstraint(canonical);
    // updated_exprs_.insert(canonical);
    return true;
}

void Solver::addNormalConstraint(ExprRef e, bool taken) {
    if (!taken)
        e = g_expr_builder->createLNot(e);
    addConstraint(e);
}

ExprRef Solver::getRangeConstraint(ExprRef e, bool is_unsigned) {
    Kind lower_kind = is_unsigned ? Uge : Sge;
    Kind upper_kind = is_unsigned ? Ule : Sle;
    RangeSet* rs = e->getRangeSet(is_unsigned);
    if (rs == NULL)
        return NULL;

    ExprRef expr = NULL;
    for (auto i = rs->begin(), end = rs->end(); i != end; i++) {
        const llvm::APSInt& from = i->From();
        const llvm::APSInt& to = i->To();
        ExprRef bound = NULL;

        if (from == to) {
            // can simplify this case
            ExprRef imm = g_expr_builder->createConstant(from, e->bits());
            bound = g_expr_builder->createEqual(e, imm);
        } else {
            ExprRef lb_imm =
                g_expr_builder->createConstant(i->From(), e->bits());
            ExprRef ub_imm = g_expr_builder->createConstant(i->To(), e->bits());
            ExprRef lb =
                g_expr_builder->createBinaryExpr(lower_kind, e, lb_imm);
            ExprRef ub =
                g_expr_builder->createBinaryExpr(upper_kind, e, ub_imm);
            bound = g_expr_builder->createLAnd(lb, ub);
        }

        if (expr == NULL)
            expr = bound;
        else
            expr = g_expr_builder->createLOr(expr, bound);
    }

    return expr;
}

bool Solver::isInterestingJcc([[maybe_unused]] ExprRef rel_expr, bool taken,
                              ADDRINT pc) {
    bool interesting = trace_.isInterestingBranch(pc, taken);
    // record for other decision
    last_interested_ = interesting;
    return interesting;
}

void Solver::negatePath(ExprRef e, bool taken) {
    reset();

    auto start = std::chrono::high_resolution_clock::now();
    syncConstraints(e);
    auto end = std::chrono::high_resolution_clock::now();
    sync_constraints_time_ = end - start;
    std::cerr << "SMT :{ \"sync_constraints_time\" : "
              << sync_constraints_time_.count() << " }\n";

    addToSolver(e, !taken);
    ++added_constraints;

    bool sat = checkAndSave();

    if (!sat) { // optimistic solving
        reset();
        addToSolver(e, !taken);
        ++added_constraints;
        checkAndSave("optimistic");
    }
}

void Solver::solveOne(z3::expr z3_expr) {
    push();
    add(z3_expr);
    checkAndSave();
    pop();
}

inline void Solver::checkFeasible() {
#ifdef CONFIG_TRACE
    if (check() == z3::unsat)
        LOG_FATAL("Infeasible constraints: " + solver_.to_smt2() + "\n");
#endif
}

} // namespace symcc
