#ifndef bytecode_h
#define bytecode_h

#include "../commontypes.h"
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
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP,
    OP_JUMP_BACK,
    OP_XOR,
    OP_CALL,
    OP_INDEXING_GET,
    OP_INDEXING_SET,
    OP_CLOSURE,
    OP_CLOSURE_LONG,
    OP_UPVALUE_GET,
    OP_UPVALUE_GET_LONG,
    OP_UPVALUE_SET,
    OP_UPVALUE_SET_LONG,
    OP_CLOSE_UPVALUE,
    OP_ARRAY,
    OP_ARRAY_LONG,
    OP_DICT,
    OP_DICT_LONG,
} OpCode;

struct sBytecode {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    LineArray lines;
};

void initBytecode(struct sBytecode* bytecode);
int writeBytecode(Collector* collector, struct sBytecode* bytecode, uint8_t byte, int line);
void freeBytecode(Collector* collector, struct sBytecode* bytecode);
int writeVariableSizeOp(Collector* collector, struct sBytecode* bytecode, OpCode oplong, OpCode opshort, uint16_t argument, int line);
int writeAddressableInstruction(Collector* collector, struct sBytecode* bytecode, OpCode oplong, OpCode opshort, Value val, int line);
void markBytecode(Collector* collector, struct sBytecode* bytecode);

#endif
