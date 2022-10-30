#ifndef SYMCC_SOLVER_H_
#define SYMCC_SOLVER_H_

#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <z3++.h>

#include "afl_trace_map.h"
#include "expr.h"
// #include "thread_context.h"
#include "expr_builder.h"

namespace symcc {
// @FIXME(alekum 29/09/2022): If context is supposed to be a global
// consider to create a singleton  that supposed to provide such context
// or factory to create such objects...
extern z3::context* g_z3_context;

class Solver {
  public:
    explicit Solver(const std::vector<uint8_t>& ibuf, const std::string out_dir,
                    const std::string bitmap, unsigned kSolverTimeout);
    ~Solver() noexcept = default;
    Solver(const Solver&) = delete;
    Solver(Solver&&) = delete;
    Solver& operator=(const Solver&) = delete;
    Solver& operator=(Solver&&) = delete;

    void push();
    void reset();
    void pop();
    void add(z3::expr expr);
    z3::check_result check();
    bool checkAndSave(const std::string& postfix = "");
    void addJcc(ExprRef, bool, ADDRINT);
    void addAddr(ExprRef, ADDRINT);
    void addAddr(ExprRef, llvm::APInt);
    void addValue(ExprRef, ADDRINT);
    void addValue(ExprRef, llvm::APInt);
    void solveAll(ExprRef, llvm::APInt);
    uint8_t getInput(ADDRINT index);
    ADDRINT last_pc() { return last_pc_; }

  protected:
    std::string input_file_;
    std::vector<uint8_t> inputs_;
    std::string out_dir_;
    z3::context& context_;
    z3::solver solver_;
    std::string session_;
    AflTraceMap trace_;
    bool last_interested_;
    bool syncing_;
    ADDRINT last_pc_;
    DependencyForest<Expr> dep_forest_;

    // @Cleanup(alekum 26/10/2022)
    // stats to be printed in print_stats method
    // turn into Solver::Stats?
    uint32_t num_generated_;
    std::chrono::duration<double> solver_check_time_;
    std::chrono::duration<double> sync_constraints_time_;
    uint32_t skipped_constraints;
    uint32_t added_constraints;
    uint32_t symbolic_variables;
    uint32_t concrete_variables;
    /// ------------------- end Solver::Stats

    std::vector<uint8_t> getConcreteValues();
    void saveValues(const std::string& postfix);
    void printValues(const std::vector<uint8_t>& values);
    // @Cleanup(alekum 26/10/2022):
    // We wanna be generic as much as possible, though
    // let's keep it simple first - consider to write to file, to screen
    void saveStats() noexcept;

    z3::expr getPossibleValue(z3::expr& z3_expr);
    z3::expr getMinValue(z3::expr& z3_expr);
    z3::expr getMaxValue(z3::expr& z3_expr);

    void addToSolver(ExprRef e, bool taken);
    void syncConstraints(ExprRef e);

    void addConstraint(ExprRef e, bool taken, bool is_interesting);
    void addConstraint(ExprRef e);
    bool addRangeConstraint(ExprRef, bool);
    void addNormalConstraint(ExprRef, bool);

    ExprRef getRangeConstraint(ExprRef e, bool is_unsigned);

    bool isInterestingJcc(ExprRef, bool, ADDRINT);
    void negatePath(ExprRef, bool);
    void solveOne(z3::expr);

    void checkFeasible();
};

extern Solver* g_solver;

} // namespace symcc

#endif // SYMCC_SOLVER_H_
