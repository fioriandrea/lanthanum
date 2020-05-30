#include "chunk.h"
#include "../util.h"
#include "../memory.h"

void initChunk(struct sChunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants); 
    initLineArray(&chunk->lines);
}

int writeChunk(Collector* collector, struct sChunk* chunk, uint8_t byte, int line) {
    if (chunk->count + 1 >= chunk->capacity) {
        int newcap = compute_capacity(chunk->capacity);
        chunk->code = grow_array(collector, uint8_t, chunk->code, chunk->capacity, newcap);
        chunk->capacity = newcap; 
    }     
    chunk->code[chunk->count++] = byte;
    writeLineArray(collector, &chunk->lines, line);
    return chunk->count - 1;
}

void freeChunk(Collector* collector, struct sChunk* chunk) {
    free_array(collector, uint8_t, chunk->code, chunk->capacity);
    freeValueArray(collector, &chunk->constants);
    freeLineArray(collector, &chunk->lines);
    initChunk(chunk);
}

int writeVariableSizeOp(Collector* collector, struct sChunk* chunk, OpCode oplong, OpCode opshort, uint16_t argument, int line) {
    if (argument > UINT8_MAX) {
        SplittedLong lng = split_long(argument);
        writeChunk(collector, chunk, oplong, line);
        writeChunk(collector, chunk, (uint8_t) lng.b0, line);
        writeChunk(collector, chunk, (uint8_t) lng.b1, line);
    } else {
        writeChunk(collector, chunk, opshort, line);
        writeChunk(collector, chunk, (uint8_t) argument, line);
    }
    return chunk->count - 1;
}

int writeAddressableInstruction(Collector* collector, struct sChunk* chunk, OpCode oplong, OpCode opshort, Value val, int line) {
    uint16_t address = (uint16_t) writeValueArray(collector, &chunk->constants, val);
    return writeVariableSizeOp(collector, chunk, oplong, opshort, address, line);
}
