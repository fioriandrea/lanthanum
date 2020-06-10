#ifndef compiler_h
#define compiler_h

#include "../datastructs/chunk.h"
#include "../memory.h"
#include "lexer.h"
#include "../datastructs/hash_map.h"

#define MAX_LOCALS 700
#define MAX_UPVALUES 700
#define MAX_LOOP_SKIPS 700

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

typedef enum {
    SKIP_BREAK,
    SKIP_CONTINUE,
} SkipType;

typedef struct {
    int address;
    int loopDepth;
    SkipType type;
} LoopSkip;

struct sScope {
    struct sScope* enclosing;
    int depth;
    Local locals[MAX_LOCALS];
    int localsCount;
    ObjFunction* function;
    Upvalue upvalues[MAX_UPVALUES];
    LoopSkip loopSkips[MAX_LOOP_SKIPS];
    int loopSkipCount;
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
