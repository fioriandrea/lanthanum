#ifndef chunk_h
#define chunk_h

#include "../commontypes.h"
#include "value.h"
#include "line_array.h"
#include "../util.h"

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
    OP_PRINT,
    OP_GLOBAL_DECL,
    OP_GLOBAL_DECL_LONG,
    OP_GLOBAL_GET,
    OP_GLOBAL_GET_LONG,
    OP_GLOBAL_SET,
    OP_GLOBAL_SET_LONG,
    OP_LOCAL_GET,
    OP_LOCAL_GET_LONG,
    OP_LOCAL_SET,
    OP_LOCAL_SET_LONG,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    LineArray lines;
} Chunk;

void initChunk(Chunk* chunk);
int writeChunk(Collector* collector, Chunk* chunk, uint8_t byte, int line);
void freeChunk(Collector* collector, Chunk* chunk);
int writeVariableSizeOp(Collector* collector, Chunk* chunk, OpCode oplong, OpCode opshort, uint16_t argument, int line);
int writeAddressableInstruction(Collector* collector, Chunk* chunk, OpCode oplong, OpCode opshort, Value val, int line);

#endif
