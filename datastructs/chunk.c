#include "chunk.h"
#include "../services/memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants); 
    initLineArray(&chunk->lines);
}

int writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->count + 1 >= chunk->capacity) {
        int newcap = compute_capacity(chunk->capacity);
        chunk->code = grow_array(uint8_t, chunk->code, chunk->capacity, newcap);
        chunk->capacity = newcap; 
    }     
    chunk->code[chunk->count++] = byte;
    writeLineArray(&chunk->lines, line);
    return chunk->count - 1;
}

void freeChunk(Chunk* chunk) {
    free_array(uint8_t, chunk->code, chunk->capacity);
    freeValueArray(&chunk->constants);
    freeLineArray(&chunk->lines);
    initChunk(chunk);
}

int writeConstant(Chunk* chunk, Value value) {
    return writeValueArray(&chunk->constants, value);
}
