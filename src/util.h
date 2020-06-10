#ifndef yaspl_util_h
#define yaspl_uitl_h

#include <string.h>
#include <stdio.h>

#include "commontypes.h"

typedef struct {
    uint8_t b0;
    uint8_t b1;
} SplittedLong;

#define join_bytes(b0, b1) ((uint16_t) (((uint16_t) (b0)) << 8) | ((uint16_t) (b1)))
static inline SplittedLong split_long(uint16_t l) {
    return (SplittedLong) {.b0 = (uint8_t) ((l & 0xff00) >> 8), .b1 = (uint8_t) (l & 0xff)};
}

static inline uint32_t hash_int(uint32_t a) {
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

static inline uint32_t hash_string(char* key, int length) {
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {                     
        hash ^= key[i];                                      
        hash *= 16777619;                                    
    }                                                      

    return hash;                                           
}     
#define hash_pointer(p) hash_int((uintptr_t) (p))
#define hash_double(v) hash_int(float_to_bits((float) v))

static inline uint32_t float_to_bits(float v) {
    uint32_t i = 0;
    memcpy(&i, &v, sizeof(float));
    return i;
}

static inline int is_integer(double n) {
    return (int) n == n;
} 

static uint32_t nearest_bigger_pow_of_two(uint32_t n) {
    if ((n & (n - 1)) == 0)
        return n;

    while ((n & (n - 1)) != 0) {
        n = n & (n - 1);
    }
    return n << 1;   
}

#endif
