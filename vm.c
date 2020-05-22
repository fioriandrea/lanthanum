#include <stdio.h>
#include <math.h>

#include "vm.h"
#include "./services/asm_printer.h"
#include "compiler.h"

#define TRACE_EXEC
#define PRINT_CODE

static int isInteger(double n) {
    return (int) n == n;
}

static void resetStack(VM* vm) {
    vm->sp = vm->stack;
}

void initVM(VM* vm) {
    vm->chunk = NULL;
    vm->pc = NULL;
    resetStack(vm);
}

static Value peek(VM* vm, int depth) {
    return vm->sp[-(depth + 1)];
}

static void push(VM* vm, Value val) {
    *vm->sp = val;
    vm->sp++;
}

static Value pop(VM* vm) {
    vm->sp--;
    return *vm->sp;
}

static ExecutionResult vmRun(VM* vm) {
#define read_byte() (*(vm->pc++))
#define read_constant() (vm->chunk->constants.values[read_byte()])
#define binary_op(operator) \
    do { \
        Value b = pop(vm); \
        Value a = pop(vm); \
        push(vm, a operator b); \
    } while (0)


#ifdef PRINT_CODE
    printf("VM CODE:\n");
    printChunk(vm->chunk, "code");
    printf("\n");
    for (int i = 0; i < vm->chunk->lines.count; i++) {
        printf("{l: %d, c: %d}\n", vm->chunk->lines.lines[i].line, vm->chunk->lines.lines[i].count);
    }
    printf("\n");
#endif

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
            case OP_NEGATE:
                {
                    Value a = pop(vm);
                    push(vm, -a);
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

ExecutionResult vmExecute(VM* vm, char* source) {
    Chunk chunk;
    initChunk(&chunk);
    Compiler compiler;
    initCompiler(&compiler, source);
    int compileResult = compile(&compiler, &chunk);
    ExecutionResult result;
    if (compileResult) {
        vm->chunk = &chunk;
        vm->pc = chunk.code;
        result = vmRun(vm);
    } else {
        result = EXEC_COMPILE_ERROR;
    }
    freeCompiler(&compiler);
    freeChunk(&chunk);
    return result;
}

void freeVM(VM* vm) {

}
