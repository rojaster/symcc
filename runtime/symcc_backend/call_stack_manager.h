#ifndef SYMCC_CALL_STACK_MANAGER_H
#define SYMCC_CALL_STACK_MANAGER_H

#include "common.h"
#include <cstdint>
#include <vector>

namespace symcc {
class CallStackManager {
  public:
    CallStackManager();
    ~CallStackManager();

    void visitCall(ADDRINT pc);
    void visitRet(ADDRINT pc);
    void visitBasicBlock(ADDRINT pc);
    void updateBitmap();
    bool isInteresting() { return is_interesting_; }

  private:
    std::vector<ADDRINT> call_stack_;
    XXH32_hash_t call_stack_hash_;
    bool is_interesting_;
    uint16_t* bitmap_;
    // [[maybe_unused]] uint32_t last_index_;
    bool pending_;
    ADDRINT last_pc_;

    void computeHash();
};

extern CallStackManager g_call_stack_manager;

} // namespace symcc

#endif // SYMCC_CALL_STACK_MANAGER_H
