#ifndef yaspl_util_h
#define yaspl_uitl_h

#include <string.h>
#include <stdio.h>

#include "standardtypes.h"

union DoubleInt {
    double dValue;
    uint64_t iValue;
};

#define hash_int(i) ((i) * UINT32_C(2654435769))
#define hash_pointer(p) hash_int((uint32_t) (p))
#define hash_double(v) hash_int((uint32_t) (v))

static inline uint64_t double_to_bits(double v) {
    // todo
    return 0;
}

static inline int is_integer(double n) {
    return (int) n == n;
}

#endif
