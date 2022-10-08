#ifndef __AFL_TRACE_MAP_H__
#define __AFL_TRACE_MAP_H__

#define FFL(_b) (0xffULL << ((_b) << 3))
#define FF(_b) (0xff << ((_b) << 3))

#include "common.h"

namespace symcc {

class AflTraceMap {

  private:
    std::string path_;
    ADDRINT prev_loc_;
    uint8_t* trace_map_;
    uint8_t* virgin_map_;
    uint8_t* context_map_;
    std::set<ADDRINT> visited_;

    void allocMap();
    void setDefault();
    void import(const std::string path);
    void commit();
    ADDRINT getIndex(ADDRINT h);
    bool isInterestingContext(ADDRINT h, ADDRINT bits);

  public:
    AflTraceMap(const std::string path);
    bool isInterestingBranch(ADDRINT pc, bool taken);
};
} // namespace symcc
#endif // __AFL_TRACE_MAP_H__
