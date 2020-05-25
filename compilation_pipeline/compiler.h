#ifndef compiler_h
#define compiler_h

#include "../datastructs/chunk.h"
#include "../memory.h"
#include "lexer.h"
#include "../datastructs/hash_map.h"


typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    Collector* collector;
    Chunk* compilingChunk;
    int hadError;
    int panic;
} Compiler;

void initCompiler(Compiler* compiler);
int compile(Compiler* compiler, Collector* collector, Chunk* chunk, char* source);
void freeCompiler(Compiler* compiler);

#endif
