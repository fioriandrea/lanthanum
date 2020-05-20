#ifndef chunk_h
#define chunk_h

#include "../standardtypes.h"
#include "value.h"
#include "line_array.h"

typedef enum {
    OP_RETURN,
    OP_CONST,
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
