#ifndef vm_h
#define vm_h

#include "./datastructs/bytecode.h"
#include "./commontypes.h"
#include "./datastructs/value.h"
#include "./datastructs/hash_map.h"

#define MAX_FRAMES 256                       
#define MAX_STACK (MAX_FRAMES * UINT8_MAX)

typedef struct {
    ObjClosure* closure;
    uint8_t* pc;
    Value* localStack;
} CallFrame;

struct sVM {
    CallFrame frames[MAX_FRAMES];
    int fp;
    Value stack[MAX_STACK];
    Value* sp;
    Collector* collector;
    HashMap globals;
    ObjUpvalue* openUpvalues;
};

void initVM(struct sVM* vm);
int vmExecute(struct sVM* vm, Collector* collector, ObjFunction* function);  
void vmDeclareNative(struct sVM* vm, int arity, char* name, CNativeFunction cfunction);
void freeVM(struct sVM* vm);
void vmPush(struct sVM* vm, Value val);
Value vmPop(struct sVM* vm);

#endif
