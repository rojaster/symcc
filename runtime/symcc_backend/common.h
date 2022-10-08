#ifndef SYMCC_COMMON_H_
#define SYMCC_COMMON_H_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <sys/mman.h>

using ADDRINT = uintptr_t;

#include "allocation.h"
#include "compiler.h"
#include "logging.h"
#include <llvm/ADT/APSInt.h>

#define EXPR_COMPLEX_LEVEL_THRESHOLD 4
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"
#endif
