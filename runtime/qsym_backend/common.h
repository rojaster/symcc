#ifndef QSYM_COMMON_H_
#define QSYM_COMMON_H_

#include "pin.H"
#include "logging.h"
#include "compiler.h"
#include "allocation.h"
#include <llvm/ADT/APSInt.h>

#define EXPR_COMPLEX_LEVEL_THRESHOLD 4
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"

#endif
