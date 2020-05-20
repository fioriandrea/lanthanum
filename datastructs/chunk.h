#ifndef chunk_h
#define chunk_h

#include "../standardtypes.h"
#include "value.h"
#include "line_array.h"

typedef enum {
    OP_RET,
    OP_CONST,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
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
