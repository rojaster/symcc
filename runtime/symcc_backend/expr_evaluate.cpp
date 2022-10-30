#include "expr.h"
#include "expr_builder.h"
#include "solver.h"

namespace symcc {
// Use ConstantFoldingExprBuilder for evaluation
static ExprBuilder* CEB = ConstantFoldingExprBuilder::create();

inline ExprRef checkExpr(ExprRef e) {
    Kind kind = e->kind();
    SYMCC_ASSERT(kind == Constant || kind == Bool);
    return e;
}

ExprRef Expr::evaluate() {
    if (isInvalidated() || evaluation_ == NULL)
        evaluation_ = evaluateImpl();
    return evaluation_;
}

ExprRef ConcatExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    ExprRef c1 = getChild(1)->evaluate();
    return checkExpr(CEB->createConcat(c0, c1));
}

ExprRef UnaryExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    return checkExpr(CEB->createUnaryExpr(kind_, c0));
}

ExprRef ReadExpr::evaluateImpl() {
    return std::make_shared<ConstantExpr>(g_solver->getInput(index_), 8);
}

ExprRef ConstantExpr::evaluateImpl() {
    return std::make_shared<ConstantExpr>(this->value(), this->bits());
}

ExprRef BoolExpr::evaluateImpl() {
    return std::make_shared<BoolExpr>(this->value());
}

ExprRef BinaryExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    ExprRef c1 = getChild(1)->evaluate();
    return checkExpr(CEB->createBinaryExpr(kind_, c0, c1));
}

ExprRef ExtractExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    return checkExpr(CEB->createExtract(c0, index_, bits_));
}

ExprRef ZExtExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    return checkExpr(CEB->createZExt(c0, bits_));
}

ExprRef SExtExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    return checkExpr(CEB->createSExt(c0, bits_));
}

ExprRef IteExpr::evaluateImpl() {
    ExprRef c0 = getChild(0)->evaluate();
    ExprRef c1 = getChild(1)->evaluate();
    ExprRef c2 = getChild(2)->evaluate();
    return checkExpr(CEB->createIte(c0, c1, c2));
}

} // namespace symcc