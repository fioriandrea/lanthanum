#ifndef compiler_h
#define compiler_h

#include "./datastructs/chunk.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
} Compiler;

void initCompiler(Compiler* compiler, char* source);
void compile(Compiler* compiler, Chunk* chunk);
void freeCompiler(Compiler* compiler);

#endif
