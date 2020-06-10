#ifndef compiler_h
#define compiler_h

#include "../datastructs/chunk.h"
#include "../memory.h"
#include "lexer.h"
#include "../datastructs/hash_map.h"

#define MAX_LOCALS 700
#define MAX_UPVALUES 700
#define MAX_SKIP_LOOPS 700

struct sLocal {
    Token name;
    int depth;
    int isCaptured;
};

struct sUpvalue {
    int index;
    int ownedAbove;
};

typedef struct sLocal Local;
typedef struct sUpvalue Upvalue;

struct sScope {
    struct sScope* enclosing;
    int depth;
    Local locals[MAX_LOCALS];
    int localsCount;
    ObjFunction* function;
    Upvalue upvalues[MAX_UPVALUES];
    int skipLoopAddresses[MAX_SKIP_LOOPS];
    int skipLoopsCount;
    int loopDepth;
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
