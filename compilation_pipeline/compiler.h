#ifndef compiler_h
#define compiler_h

#include "../datastructs/chunk.h"
#include "../memory.h"
#include "lexer.h"
#include "../datastructs/hash_map.h"

struct sLocal {
    Token name;
    int depth;
};

typedef struct sLocal Local;

struct sScope {
    struct sScope* enclosing;
    int depth;
    Local locals[256]; // todo: change constant
    int count;
    ObjFunction* function;
};

typedef struct sScope Scope;

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    Collector* collector;
    int hadError;
    int panic;
    Scope *scope;
} Compiler;

void initCompiler(Compiler* compiler);
ObjFunction* compile(Compiler* compiler, Collector* collector, char* source);
void freeCompiler(Compiler* compiler);

#endif
