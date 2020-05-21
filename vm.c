#include <stdio.h>
#include <math.h>

#include "vm.h"
#include "../services/asm_printer.h"

#define TRACE_EXEC

int isInteger(double n) {
    return (int) n == n;
}

void resetStack(VM* vm) {
    vm->sp = vm->stack;
}

void initVM(VM* vm) {
    vm->chunk = NULL;
    vm->pc = NULL;
    resetStack(vm);
}

Value peek(VM* vm, int depth) {
    return vm->sp[-(depth + 1)];
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
                    if (peek(vm, 0) == 0) {
                        // runtime error;
                        return EXEC_RUNTIME_ERROR;
                    }
                    binary_op(/);
                    break;
                }
            case OP_MOD:
                {
                    if (peek(vm, 0) == 0) {
                        // runtime error;
                        return EXEC_RUNTIME_ERROR;
                    }
                    if (!isInteger(peek(vm, 0)) || !isInteger(peek(vm, 1))) {
                        // runtime error;
                        return EXEC_RUNTIME_ERROR;
                    }
                    
                    Value b = pop(vm); 
                    Value a = pop(vm); 
                    push(vm, (Value) (((long) a) % ((long) b)));
                    break;
                }
            case OP_POW:
                {
                    Value b = pop(vm);
                    Value a = pop(vm);
                    push(vm, pow(a, b));
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
