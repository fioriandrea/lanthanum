#include "chunk.h"
#include "../services/memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
}

int writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->count + 1 >= chunk->capacity) {
        int newcap = compute_capacity(chunk->capacity);
        chunk->code = grow_array(uint8_t, chunk->code, chunk->capacity, newcap);
        chunk->capacity = newcap; 
    }     
    chunk->code[chunk->count++] = byte;
}

void freeChunk(Chunk* chunk) {
    free_array(uint8_t, chunk->code, chunk->capacity);
    initChunk(chunk);
}
