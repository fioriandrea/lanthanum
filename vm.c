#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"
#include "./services/asm_printer.h"
#include "./services/memory.h"
#include "./datastructs/value.h"
#include "compiler.h"

#define TRACE_EXEC
#define PRINT_CODE

static int isInteger(double n) {
    return (int) n == n;
}

static int isTruthy(Value val) {
    return !(is_nihl(val) || (is_bool(val) && !as_cbool(val)) ||
            (is_number(val) && as_cnumber(val) == 0));
}

static int valuesIntegers(Value a, Value b) {
    return isInteger(as_cnumber(a)) && isInteger(as_cnumber(b));
}

static int valuesEqual(Value a, Value b) {
    if (a.type != b.type)
        return 0;
    switch (a.type) {
        case VALUE_NIHL: return 1;
        case VALUE_NUMBER: return as_cnumber(a) == as_cnumber(b);
        case VALUE_BOOL: return as_cbool(a) == as_cbool(b);
        case VALUE_OBJ: return as_obj(a) == as_obj(b);
    }
}

static int valuesConcatenable(Value a, Value b) {
    return is_string(a) && is_string(b);
}

static int valuesNumbers(Value a, Value b) {
    return is_number(a) && is_number(b);
}

static Value concatenate(Value a, Value b) {
    ObjString* sa = as_string(a);
    ObjString* sb = as_string(b);
    int length = sa->length + sb->length;
    char* chars = allocate_block(char, length);
    memcpy(chars, sa->chars, sa->length);
    memcpy(chars + sa->length, sb->chars, sb->length);
    chars[length] = '\0'; 
    return to_vobj(takeString(chars, length));
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

static void runtimeError(VM* vm, char* format, ...) {
    int instruction = vm->pc - vm->chunk->code - 1; 
    int line = lineArrayGet(&vm->chunk->lines, instruction);         

    va_list args;                                    
    va_start(args, format);                          
    fprintf(stderr, "runtime error [line %d] in script: ", line);  
    vfprintf(stderr, format, args);                  
    va_end(args);                                    
    fputs("\n", stderr);
    resetStack(vm);                                    
}    

static ExecutionResult vmRun(VM* vm) {
#define read_byte() (*(vm->pc++))
#define read_constant() (vm->chunk->constants.values[read_byte()])
#define binary_op(operator, destination) \
    do { \
        if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) { \
            runtimeError(vm, "operand must be numbers"); \
            return EXEC_RUNTIME_ERROR; \
        } \
        double b = as_cnumber(pop(vm)); \
        double a = as_cnumber(pop(vm)); \
        push(vm, destination(a operator b)); \
    } while (0)


#ifdef PRINT_CODE
    printf("VM CODE:\n");
    printChunk(vm->chunk, "code");
    // printf("\n");
    // for (int i = 0; i < vm->chunk->lines.count; i++) {
    //     printf("{l: %d, c: %d}\n", vm->chunk->lines.lines[i].line, vm->chunk->lines.lines[i].count);
    // }
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
                    if (!is_number(peek(vm, 0))) {
                        runtimeError(vm, "only numbers can be negated");
                        return EXEC_RUNTIME_ERROR;
                    }
                    Value a = pop(vm);
                    push(vm, to_vnumber(-as_cnumber(a)));
                    break;
                }
            case OP_ADD:
                {
                    binary_op(+, to_vnumber);
                    break;      
                }
            case OP_SUB: 
                {
                    binary_op(-, to_vnumber);
                    break;      
                }
            case OP_MUL:
                {
                    binary_op(*, to_vnumber);
                    break;      
                }
            case OP_DIV:
                {
                    if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "operands must be numbers");
                        return EXEC_RUNTIME_ERROR;
                    }
                    if (as_cnumber(peek(vm, 0)) == 0) {
                        runtimeError(vm, "cannot divide by zero (/ 0)");
                        return EXEC_RUNTIME_ERROR;
                    }
                    double b = as_cnumber(pop(vm)); 
                    double a = as_cnumber(pop(vm)); 
                    push(vm, to_vnumber(a / b));
                    break;
                }
            case OP_MOD:
                {
                    if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "operands must be numbers");
                        return EXEC_RUNTIME_ERROR;
                    }
                    if (as_cnumber(peek(vm, 0)) == 0) {
                        runtimeError(vm, "cannot divide by 0 (% 0)");
                        return EXEC_RUNTIME_ERROR;
                    }
                    if (!valuesIntegers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "only integer allowed when using %");
                        return EXEC_RUNTIME_ERROR;
                    }

                    double b = as_cnumber(pop(vm)); 
                    double a = as_cnumber(pop(vm)); 
                    push(vm, to_vnumber(((long) a) % ((long) b)));
                    break;
                }
            case OP_POW:
                {
                    if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "operands must be numbers");
                        return EXEC_RUNTIME_ERROR;
                    }
                    double b = as_cnumber(pop(vm)); 
                    double a = as_cnumber(pop(vm)); 
                    push(vm, to_vnumber(pow(a, b)));
                    break;
                }
            case OP_CONST_NIHL:
                {
                    push(vm, to_vnihl());
                    break;
                }
            case OP_CONST_TRUE:
                {
                    push(vm, to_vbool(1));
                    break;
                }
            case OP_CONST_FALSE:
                {
                    push(vm, to_vbool(0));
                    break;
                }
            case OP_NOT:
                {
                    Value val = pop(vm);
                    push(vm, to_vbool(!isTruthy(val)));
                    break;
                }
            case OP_POP:
                {
                    pop(vm);
                    break;
                }
            case OP_EQUAL:
                {
                    Value b = pop(vm);
                    Value a = pop(vm);
                    push(vm, to_vbool(valuesEqual(a, b)));
                    break;
                }
            case OP_NOT_EQUAL:
                {
                    Value b = pop(vm);
                    Value a = pop(vm);
                    push(vm, to_vbool(!valuesEqual(a, b)));
                    break;
                }
            case OP_LESS:
                {
                    binary_op(<, to_vbool);
                    break;
                }
            case OP_LESS_EQUAL:
                {
                    binary_op(<=, to_vbool);
                    break;
                }
            case OP_GREATER:
                {
                    binary_op(>, to_vbool);
                    break;
                }
            case OP_GREATER_EQUAL:
                {
                    binary_op(>=, to_vbool);
                    break;
                }
            case OP_CONCAT:
                {
                    if (!valuesConcatenable(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "values must be strings");
                        return EXEC_RUNTIME_ERROR;
                    }
                    Value b = pop(vm);
                    Value a = pop(vm);
                    push(vm, concatenate(a, b));
                    break;
                }
            default:
                {
                    runtimeError(vm, "unknown instruction");
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
