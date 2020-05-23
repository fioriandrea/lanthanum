#ifndef chunk_h
#define chunk_h

#include "../standardtypes.h"
#include "value.h"
#include "line_array.h"

typedef enum {
    OP_RET,
    OP_CONST,
    OP_CONST_LONG,
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
    OP_NOT,
    OP_CONST_NIHL,
    OP_CONST_TRUE,
    OP_CONST_FALSE,
    OP_POP,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_CONCAT,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    LineArray lines;
} Chunk;

typedef struct {
    uint8_t b0;
    uint8_t b1;
} SplittedLong;

#define join_bytes(b0, b1) ((uint16_t) (((uint16_t) (b0)) << 8) | ((uint16_t) (b1)))
static inline SplittedLong split_long(uint16_t l) {
    return (SplittedLong) {.b0 = (uint8_t) ((l & 0xff00) >> 8), .b1 = (uint8_t) (l & 0xff)};
}
void initChunk(Chunk* chunk);
int writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);
int writeConstant(Chunk* chunk, Value value, int line);

#endif
