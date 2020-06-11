#include "bytecode.h"
#include "../util.h"
#include "../memory.h"

void initBytecode(struct sBytecode* bytecode) {
    bytecode->count = 0;
    bytecode->capacity = 0;
    bytecode->code = NULL;
    initValueArray(&bytecode->constants); 
    initLineArray(&bytecode->lines);
}

int writeBytecode(Collector* collector, struct sBytecode* bytecode, uint8_t byte, int line) {
    if (bytecode->count + 1 >= bytecode->capacity) {
        int newcap = compute_capacity(bytecode->capacity);
        bytecode->code = grow_array(collector, uint8_t, bytecode->code, bytecode->capacity, newcap);
        bytecode->capacity = newcap; 
    }     
    bytecode->code[bytecode->count++] = byte;
    writeLineArray(collector, &bytecode->lines, line);
    return bytecode->count - 1;
}

void freeBytecode(Collector* collector, struct sBytecode* bytecode) {
    free_array(collector, uint8_t, bytecode->code, bytecode->capacity);
    freeValueArray(collector, &bytecode->constants);
    freeLineArray(collector, &bytecode->lines);
    initBytecode(bytecode);
}

int writeVariableSizeOp(Collector* collector, struct sBytecode* bytecode, OpCode oplong, OpCode opshort, uint16_t argument, int line) {
    if (argument > UINT8_MAX) {
        SplittedLong lng = split_long(argument);
        writeBytecode(collector, bytecode, oplong, line);
        writeBytecode(collector, bytecode, (uint8_t) lng.b0, line);
        writeBytecode(collector, bytecode, (uint8_t) lng.b1, line);
    } else {
        writeBytecode(collector, bytecode, opshort, line);
        writeBytecode(collector, bytecode, (uint8_t) argument, line);
    }
    return bytecode->count - 1;
}

int writeAddressableInstruction(Collector* collector, struct sBytecode* bytecode, OpCode oplong, OpCode opshort, Value val, int line) {
    pushSafe(collector, val);
    uint16_t address = (uint16_t) writeValueArray(collector, &bytecode->constants, val);
    int result = writeVariableSizeOp(collector, bytecode, oplong, opshort, address, line);
    popSafe(collector);
    return result;
}

void markBytecode(Collector* collector, struct sBytecode* bytecode) {
    markValueArray(collector, &bytecode->constants);
}
