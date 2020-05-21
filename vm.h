#ifndef vm_h
#define vm_h

#include "./datastructs/chunk.h"
#include "./standardtypes.h"

#define MAX_STACK 256

typedef struct {
    Chunk* chunk;
    uint8_t* pc;
    Value stack[MAX_STACK];
    Value* sp;
} VM;

typedef enum {
    EXEC_OK,
    EXEC_RUNTIME_ERROR,
    EXEC_COMPILE_ERROR,
} ExecutionResult;

void initVM(VM* vm);
ExecutionResult vmExecute(VM* vm, Chunk* chunk);  
void freeVM(VM* vm);

#endif
