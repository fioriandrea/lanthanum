#ifndef vm_h
#define vm_h

#include "./datastructs/chunk.h"
#include "./commontypes.h"
#include "./datastructs/value.h"

#define MAX_STACK 256

typedef struct {
    Chunk* chunk;
    uint8_t* pc;
    Value stack[MAX_STACK];
    Value* sp;
    Collector* collector;
} VM;

typedef enum {
    EXEC_OK,
    EXEC_RUNTIME_ERROR,
    EXEC_COMPILE_ERROR,
} ExecutionResult;

void initVM(VM* vm, Collector* collector);
ExecutionResult vmExecute(VM* vm, char* source);  
void freeVM(VM* vm);

#endif
