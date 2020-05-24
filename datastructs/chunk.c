#include "chunk.h"
#include "../memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants); 
    initLineArray(&chunk->lines);
}

int writeChunk(Collector* collector, Chunk* chunk, uint8_t byte, int line) {
    if (chunk->count + 1 >= chunk->capacity) {
        int newcap = compute_capacity(chunk->capacity);
        chunk->code = grow_array(collector, uint8_t, chunk->code, chunk->capacity, newcap);
        chunk->capacity = newcap; 
    }     
    chunk->code[chunk->count++] = byte;
    writeLineArray(collector, &chunk->lines, line);
    return chunk->count - 1;
}

void freeChunk(Collector* collector, Chunk* chunk) {
    free_array(collector, uint8_t, chunk->code, chunk->capacity);
    freeValueArray(collector, &chunk->constants);
    freeLineArray(collector, &chunk->lines);
    initChunk(chunk);
}

int writeConstant(Collector* collector, Chunk* chunk, Value value, int line) {
    uint16_t address = (uint16_t) writeValueArray(collector, &chunk->constants, value);
    if (address > UINT8_MAX) {
        SplittedLong lng = split_long(address);
        writeChunk(collector, chunk, OP_CONST_LONG, line);
        writeChunk(collector, chunk, (uint8_t) lng.b0, line);
        writeChunk(collector, chunk, (uint8_t) lng.b1, line);
    } else {
        writeChunk(collector, chunk, OP_CONST, line);
        writeChunk(collector, chunk, (uint8_t) address, line);
    }
    return chunk->count - 1;
}
