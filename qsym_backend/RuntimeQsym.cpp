//
// Definitions that we need for the Qsym backend
//

#include "Runtime.h"

// C++
#include <fstream>
#include <iterator>
#include <numeric>

// C
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Qsym
#include <afl_trace_map.h>
#include <call_stack_manager.h>
#include <expr_builder.h>
#include <solver.h>

// Runtime
#include <Shadow.h>

namespace qsym {

ExprBuilder *g_expr_builder;
Solver *g_solver;
CallStackManager g_call_stack_manager;
z3::context g_z3_context;

AflTraceMap::AflTraceMap(const std::string path) {}

bool AflTraceMap::isInterestingBranch(ADDRINT pc, bool taken) {
  // TODO
  return true;
}

} // namespace qsym

using namespace qsym;

void _sym_initialize(void) {
  // TODO proper input file
  // TODO proper output directory

  // Qsym requires the full input in a file
  std::istreambuf_iterator<char> in_begin(std::cin), in_end;
  std::vector<char> inputData(in_begin, in_end);
  std::ofstream inputFile("/tmp/input", std::ios::trunc);
  std::copy(inputData.begin(), inputData.end(),
            std::ostreambuf_iterator<char>(inputFile));
  inputFile.close();

  // Restore some semblance of standard input
  int inputFd = open("/tmp/input", O_RDONLY);
  if (inputFd < 0) {
    perror("Failed to open the input file");
    exit(-1);
  }

  if (dup2(inputFd, STDIN_FILENO) < 0) {
    perror("Failed to redirect standard input");
    exit(-1);
  }

  g_solver = new Solver("/tmp/input"s, "/tmp/output"s, "fake"s);
  g_expr_builder = SymbolicExprBuilder::create();
}

SymExpr _sym_build_integer(uint64_t value, uint8_t bits) {
  return new ExprRef(g_expr_builder->createConstant(value, bits));
}

// SymExpr _sym_build_float(double value, int is_double);
// SymExpr _sym_build_variable(const char *name, uint8_t bits);
// SymExpr _sym_build_null_pointer();
// SymExpr _sym_build_true();
// SymExpr _sym_build_false();
// SymExpr _sym_build_bool(bool value);
// SymExpr _sym_build_add(SymExpr a, SymExpr b);
// SymExpr _sym_build_sub(SymExpr a, SymExpr b);
// SymExpr _sym_build_mul(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_div(SymExpr a, SymExpr b);
// SymExpr _sym_build_signed_div(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_rem(SymExpr a, SymExpr b);
// SymExpr _sym_build_signed_rem(SymExpr a, SymExpr b);
// SymExpr _sym_build_shift_left(SymExpr a, SymExpr b);
// SymExpr _sym_build_logical_shift_right(SymExpr a, SymExpr b);
// SymExpr _sym_build_arithmetic_shift_right(SymExpr a, SymExpr b);

// SymExpr _sym_build_fp_add(SymExpr a, SymExpr b);
// SymExpr _sym_build_fp_sub(SymExpr a, SymExpr b);
// SymExpr _sym_build_fp_mul(SymExpr a, SymExpr b);
// SymExpr _sym_build_fp_div(SymExpr a, SymExpr b);
// SymExpr _sym_build_fp_rem(SymExpr a, SymExpr b);
// SymExpr _sym_build_fp_abs(SymExpr a);
// SymExpr _sym_build_neg(SymExpr expr);
// SymExpr _sym_build_signed_less_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_signed_less_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_signed_greater_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_signed_greater_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_less_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_less_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_greater_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_unsigned_greater_equal(SymExpr a, SymExpr b);

SymExpr _sym_build_equal(SymExpr a, SymExpr b) {
  return new ExprRef(g_expr_builder->createEqual(*a, *b));
}

SymExpr _sym_build_not_equal(SymExpr a, SymExpr b) {
  return new ExprRef(g_expr_builder->createDistinct(*a, *b));
}

// SymExpr _sym_build_bool_and(SymExpr a, SymExpr b);

SymExpr _sym_build_and(SymExpr a, SymExpr b) {
  return new ExprRef(g_expr_builder->createAnd(*a, *b));
}

// SymExpr _sym_build_bool_or(SymExpr a, SymExpr b);
// SymExpr _sym_build_or(SymExpr a, SymExpr b);
// SymExpr _sym_build_bool_xor(SymExpr a, SymExpr b);
// SymExpr _sym_build_xor(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_greater_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_greater_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_less_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_less_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_ordered_not_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_greater_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_greater_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_less_than(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_less_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_float_unordered_not_equal(SymExpr a, SymExpr b);
// SymExpr _sym_build_sext(SymExpr expr, uint8_t bits);
// SymExpr _sym_build_zext(SymExpr expr, uint8_t bits);
// SymExpr _sym_build_trunc(SymExpr expr, uint8_t bits);
// SymExpr _sym_build_int_to_float(SymExpr value, int is_double, int is_signed);
// SymExpr _sym_build_float_to_float(SymExpr expr, int to_double);
// SymExpr _sym_build_bits_to_float(SymExpr expr, int to_double);
// SymExpr _sym_build_float_to_bits(SymExpr expr);
// SymExpr _sym_build_float_to_signed_integer(SymExpr expr, uint8_t bits);
// SymExpr _sym_build_float_to_unsigned_integer(SymExpr expr, uint8_t bits);

void _sym_push_path_constraint(SymExpr constraint, int taken) {
  if (!constraint)
    return;

  // TODO program counter
  g_solver->addJcc(*constraint, taken, 42);
}

SymExpr _sym_get_input_byte(size_t offset) {
  return new ExprRef(g_expr_builder->createRead(offset));
}

// TODO unify
SymExpr _sym_read_memory(uint8_t *addr, size_t length, bool little_endian) {
  assert(length && "Invalid query for zero-length memory region");

#ifdef DEBUG_RUNTIME
  std::cout << "Reading " << length << " bytes from address " << P(addr)
            << std::endl;
  dump_known_regions();
#endif

  // If the entire memory region is concrete, don't create a symbolic expression
  // at all.
  if (isConcrete(addr, length))
    return nullptr;

  ReadOnlyShadow shadow(addr, length);
  return std::accumulate(
      shadow.begin_non_null(), shadow.end_non_null(),
      static_cast<SymExpr>(nullptr), [&](SymExpr result, SymExpr byteExpr) {
        if (!result)
          return byteExpr;

        return new ExprRef(
            little_endian ? g_expr_builder->createConcat(*byteExpr, *result)
                          : g_expr_builder->createConcat(*result, *byteExpr));
      });
}

// void _sym_write_memory(uint8_t *addr, size_t length, SymExpr expr,
//                        bool little_endian);
// SymExpr _sym_build_extract(SymExpr expr, uint64_t offset, uint64_t length,
//                           bool little_endian);