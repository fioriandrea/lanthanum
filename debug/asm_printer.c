#include <stdio.h>

#include "asm_printer.h"
#include "../commontypes.h"
#include "../datastructs/value.h"
#include "../datastructs/object.h"

static int printSimpleInstruction(char* instname, int offset) {
    printf("%s\n", instname);
    return offset + 1;
}

static int printConstantInstruction(char* instname, Chunk* chunk, int offset) {
    uint8_t address = chunk->code[offset + 1];
    Value* val = &chunk->constants.values[address];
    printf("%s [%d] '", instname, address);
    printValue(*val);
    printf("'\n");
    return offset + 2;
}
static int printLongConstantInstruction(char* instname, Chunk* chunk, int offset) {
    uint16_t address = join_bytes(chunk->code[offset + 1], chunk->code[offset + 2]);
    Value* val = &chunk->constants.values[address];
    printf("%s [%d] '", instname, address);
    printValue(*val);
    printf("'\n");
    return offset + 3;
}

void printChunk(Chunk* chunk, char* name) {
    printf("chunk => %s\n", name);
    for (int i = 0; i < chunk->count; ) {
        i = printInstruction(chunk, chunk->code[i], i);
    }
}

int printInstruction(Chunk* chunk, OpCode code, int offset) {
#define print_simple_instruction(op) case op: return printSimpleInstruction(#op, offset);
    printf("line = %d: ", lineArrayGet(&chunk->lines, offset));
    switch (code) {
        case OP_CONST:
            return printConstantInstruction("OP_CONST", chunk, offset);
        case OP_CONST_LONG:
            return printLongConstantInstruction("OP_CONST_LONG", chunk, offset);
            print_simple_instruction(OP_RET)
                print_simple_instruction(OP_NEGATE)
                print_simple_instruction(OP_ADD)
                print_simple_instruction(OP_SUB)
                print_simple_instruction(OP_MUL)
                print_simple_instruction(OP_DIV)
                print_simple_instruction(OP_MOD)
                print_simple_instruction(OP_POW)
                print_simple_instruction(OP_CONST_TRUE)
                print_simple_instruction(OP_CONST_FALSE)
                print_simple_instruction(OP_CONST_NIHL)
                print_simple_instruction(OP_NOT)
                print_simple_instruction(OP_POP)
                print_simple_instruction(OP_LESS)
                print_simple_instruction(OP_LESS_EQUAL)
                print_simple_instruction(OP_GREATER)
                print_simple_instruction(OP_GREATER_EQUAL)
                print_simple_instruction(OP_EQUAL)
                print_simple_instruction(OP_CONCAT)
        default:
                printf("Undefined instruction: [opcode = %d]\n", code);
                return offset + 1;
    }
#undef print_simple_instruction
}
