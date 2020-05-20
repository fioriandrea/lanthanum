#include <stdio.h>

#include "asm_printer.h"
#include "../standardtypes.h"

static int printSimpleInstruction(char* instname, int offset) {
    printf("%s\n", instname);
    return offset + 1;
}

void printValue(Value val) {
    printf("%f", val);
}

static int printConstantInstruction(char* instname, Chunk* chunk, int offset) {
    uint8_t address = chunk->code[offset + 1];
    Value* val = &chunk->constants.values[address];
    printf("%s [%d] '", instname, address);
    printValue(*val);
    printf("'\n");
    return offset + 2;
}

void printChunk(Chunk* chunk, char* name) {
    printf("chunk => %s\n", name);
    for (int i = 0; i < chunk->count; ) {
        i = printInstruction(chunk, chunk->code[i], i);
    }
}

int printInstruction(Chunk* chunk, OpCode code, int offset) {
    printf("line = %d: ", lineArrayGet(&chunk->lines, offset));
    switch (code) {
        case OP_RET:
            return printSimpleInstruction("OP_RET", offset);
        case OP_CONST:
            return printConstantInstruction("OP_CONST", chunk, offset);
        case OP_ADD:
            return printSimpleInstruction("OP_ADD", offset);
        case OP_SUB:
            return printSimpleInstruction("OP_SUB", offset);
        case OP_MUL:
            return printSimpleInstruction("OP_MUL", offset);
        case OP_DIV:
            return printSimpleInstruction("OP_DIV", offset);
        case OP_MOD:
            return printSimpleInstruction("OP_MOD", offset);
        case OP_POW:
            return printSimpleInstruction("OP_POW", offset);
        default:
            printf("Undefined instruction: [opcode = %d]\n", code);
            return offset + 1;
    }
}
