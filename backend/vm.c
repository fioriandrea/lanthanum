#include <stdio.h>

#include "vm.h"
#include "../services/asm_printer.h"

#define TRACE_EXEC

void resetStack(VM* vm) {
    vm->sp = vm->stack;
}

void initVM(VM* vm) {
    vm->chunk = NULL;
    vm->pc = NULL;
    resetStack(vm);
}

void push(VM* vm, Value val) {
    *vm->sp = val;
    vm->sp++;
}

Value pop(VM* vm) {
    vm->sp--;
    return *vm->sp;
}

ExecutionResult vmRun(VM* vm) {
#define read_byte() (*(vm->pc++))
#define read_constant() (vm->chunk->constants.values[read_byte()])
#define binary_op(operator) \
    do { \
        Value b = pop(vm); \
        Value a = pop(vm); \
        push(vm, a operator b); \
    } while (0)


#ifdef TRACE_EXEC
    printf("VM EXECUTION TRACE:\n");
#endif

    for (;;) {
#ifdef TRACE_EXEC
        printf("\n");
        printInstruction(vm->chunk, *vm->pc, (int) (vm->pc - vm->chunk->code));
        printf("stack: [");
        for (Value* start = vm->stack; start < vm->sp; start++) {
            printValue(*start);
            printf(" | ");
        }
        printf("]\n");
        printf("\n");
#endif
        switch (read_byte()) {
            case OP_RET: 
                {
                    Value val = pop(vm);
                    printValue(val);
                    printf("\n");
                    return EXEC_OK;
                    break;
                }
            case OP_CONST: 
                {
                    Value constant = read_constant();
                    push(vm, constant);
                    break;
                }
            case OP_ADD:
                {
                    binary_op(+);
                    break;      
                }
            case OP_SUB: 
                {
                    binary_op(-);
                    break;      
                }
            case OP_MUL:
                {
                    binary_op(*);
                    break;      
                }
            case OP_DIV:
                {
                    binary_op(/);
                    break;
                }
            default:
                {
                    // runtime error;
                    return EXEC_RUNTIME_ERROR;
                    break;
                }
        }
    }
#undef read_byte
#undef read_constant
}

ExecutionResult vmExecute(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;
    vm->pc = chunk->code;
    return vmRun(vm);
}

void freeVM(VM* vm) {

}
