#ifndef vm_h
#define vm_h

#include "./datastructs/chunk.h"
#include "./commontypes.h"
#include "./datastructs/value.h"
#include "./datastructs/hash_map.h"

#define MAX_STACK 256

typedef struct {
    ObjFunction* function;
    uint8_t* pc;
    Value stack[MAX_STACK];
    Value* sp;
    Collector* collector;
    HashMap globals;
} VM;

void initVM(VM* vm);
int vmExecute(VM* vm, Collector* collector, ObjFunction* function);  
void freeVM(VM* vm);

#endif
