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
    switch (code) {
        case OP_RETURN:
            return printSimpleInstruction("OP_RETURN", offset);
        case OP_CONST:
            return printConstantInstruction("OP_CONSTANT", chunk, offset);
        default:
            printf("Undefined instruction: [opcode = %d]\n", code);
            return offset + 1;
    }
}
