#ifndef compiler_h
#define compiler_h

#include "../datastructs/chunk.h"
#include "../memory.h"
#include "lexer.h"


typedef struct {
    Collector* collector;
    Lexer lexer;
    Token current;
    Token previous;
    Chunk* compilingChunk;
    int hadError;
    int panic;
} Compiler;

void initCompiler(Compiler* compiler, Collector* collector, char* source);
int compile(Compiler* compiler, Chunk* chunk);
void freeCompiler(Compiler* compiler);

#endif
