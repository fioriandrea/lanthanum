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
#define hash_double(v) hash_int(float_to_bits((float) v))

static inline uint32_t float_to_bits(float v) {
    uint32_t i = 0;
    memcpy(&i, &v, sizeof(float));
    return i;
}

static inline int is_integer(double n) {
    return (int) n == n;
}

#endif
