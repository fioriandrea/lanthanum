#ifndef chunk_h
#define chunk_h

#include "../standardtypes.h"
#include "value.h"
#include "line_array.h"

typedef enum {
    OP_RET,
    OP_CONST,
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
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    LineArray lines;
} Chunk;

void initChunk(Chunk* chunk);
int writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);
int writeConstant(Chunk* chunk, Value value);

#endif
