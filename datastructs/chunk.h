#ifndef chunk_h
#define chunk_h

#include "../standardtypes.h"
#include "value.h"

typedef enum {
    OP_RETURN,
    OP_CONST,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
int writeChunk(Chunk* chunk, uint8_t byte);
void freeChunk(Chunk* chunk);
int writeConstant(Chunk* chunk, Value value);

#endif
