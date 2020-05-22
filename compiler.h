#ifndef compiler_h
#define compiler_h

#include "./datastructs/chunk.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    Chunk* compilingChunk;
    int hadError;
} Compiler;

void initCompiler(Compiler* compiler, char* source);
int compile(Compiler* compiler, Chunk* chunk);
void freeCompiler(Compiler* compiler);

#endif
