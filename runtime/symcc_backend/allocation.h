#ifndef SYMCC_ALLOCATION_H_
#define SYMCC_ALLOCATION_H_

#include "common.h"
namespace symcc {

void* allocPages(size_t, int);
void* allocRWPages(size_t);
void deallocPages(void*, size_t);
void* safeRealloc(void*, size_t);
void* safeMalloc(size_t);
void* safeCalloc(size_t, size_t);

} // namespace symcc

#endif // SYMCC_ALLOCATION_H_
