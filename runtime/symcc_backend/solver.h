#ifndef SYMCC_SOLVER_H_
#define SYMCC_SOLVER_H_

#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <z3++.h>

#include "pin.H"

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
    Solver(const std::vector<UINT8>& ibuf, const std::string out_dir,
           const std::string bitmap);

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
    UINT8 getInput(ADDRINT index);

    ADDRINT last_pc() { return last_pc_; }

  protected:
    std::string input_file_;
    std::vector<UINT8> inputs_;
    std::string out_dir_;
    z3::context& context_;
    z3::solver solver_;
    std::string session_;
    INT32 num_generated_;
    AflTraceMap trace_;
    bool last_interested_;
    bool syncing_;
    uint64_t start_time_;
    uint64_t solving_time_;
    ADDRINT last_pc_;
    DependencyForest<Expr> dep_forest_;

    std::vector<UINT8> getConcreteValues();
    void saveValues(const std::string& postfix);
    void printValues(const std::vector<UINT8>& values);

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
