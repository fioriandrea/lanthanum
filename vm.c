#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"
#include "./debug/asm_printer.h"
#include "./debug/map_printer.h"
#include "memory.h"
#include "util.h"
#include "./datastructs/value.h"
#include "./compilation_pipeline/compiler.h"

/*#define TRACE_EXEC
#define TRACE_INTERNED
#define TRACE_GLOBALS*/
#define RUNTIME_ERROR 0
#define RUNTIME_OK 1

static void resetStack(VM* vm) {
    vm->sp = vm->stack;
}

void initVM(VM* vm) {
    vm->fp = 0;
    resetStack(vm);
    initMap(&vm->globals);
}

static void runtimeError(VM* vm, char* format, ...) {
    int instruction = vm->frames[vm->fp - 1].pc - vm->frames[vm->fp - 1].function->chunk->code - 1; 
    int line = lineArrayGet(&vm->frames[vm->fp - 1].function->chunk->lines, instruction);         

    va_list args;                                    
    va_start(args, format);                          
    fprintf(stderr, "runtime error [line %d] in script: ", line);  
    vfprintf(stderr, format, args);                  
    va_end(args);                                    
    fputs("\n", stderr);
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

static int vmRun(VM* vm) {
    vm->fp = 1;
    CallFrame* currentFrame = &vm->frames[0];
#define read_byte() (*(currentFrame->pc++))
#define read_long() join_bytes(read_byte(), read_byte())
#define read_constant() (currentFrame->function->chunk->constants.values[read_byte()])
#define read_constant_long() (currentFrame->function->chunk->constants.values[read_long()])
#define binary_op(operator, destination) \
    do { \
        if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) { \
            runtimeError(vm, "operand must be numbers"); \
            return RUNTIME_ERROR; \
        } \
        double b = as_cnumber(pop(vm)); \
        double a = as_cnumber(pop(vm)); \
        push(vm, destination(a operator b)); \
    } while (0)

#ifdef TRACE_EXEC
    printf("VM EXECUTION TRACE:\n");
#endif

    for (;;) {
#ifdef TRACE_EXEC
        printf("\n");
        printInstruction(currentFrame->function->chunk, *currentFrame->pc, (int) (currentFrame->pc - currentFrame->function->chunk->code));
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
                    Value retVal = pop(vm);
                    vm->fp--;
                    if (vm->fp == 0)
                        return RUNTIME_OK;
                    vm->sp = currentFrame->localStack;
                    currentFrame = &vm->frames[vm->fp - 1];
                    push(vm, retVal);
                    break;
                }
            case OP_CALL:
                {
                    uint8_t argCount = read_byte();
                    if (argCount > (vm->sp - vm->stack)) {
                        runtimeError(vm, "too many function arguments");
                        return RUNTIME_ERROR;
                    }
                    Value functionValue = peek(vm, argCount);
                    if (!is_function(functionValue)) {
                        runtimeError(vm, "can only call functions");
                        return RUNTIME_ERROR;
                    }
                    ObjFunction* function = as_function(functionValue);
                    if (argCount != function->arity) {
                        runtimeError(vm, "expected %d arguments, got %d", function->arity, argCount);
                        return RUNTIME_ERROR;
                    }
                    if (vm->fp + 1 >= MAX_FRAMES) {             
                        runtimeError(vm, "stack overflow");             
                        return RUNTIME_ERROR;                                
                    } 
                    currentFrame = &vm->frames[vm->fp++];
                    currentFrame->function = function;
                    currentFrame->pc = currentFrame->function->chunk->code;
                    currentFrame->localStack = vm->sp - argCount;
                    break;
                }
            case OP_CONST: 
                {
                    Value constant = read_constant();
                    push(vm, constant);
                    break;
                }
            case OP_CONST_LONG:
                {
                    Value constant = read_constant_long();
                    push(vm, constant);
                    break;
                }
            case OP_GLOBAL_DECL:
                {
                    Value name = read_constant();
                    mapPut(vm->collector, &vm->globals, name, peek(vm, 0));
                    pop(vm); 
                    break;
                }
            case OP_GLOBAL_DECL_LONG:
                {
                    Value name = read_constant_long();
                    mapPut(vm->collector, &vm->globals, name, peek(vm, 0));
                    pop(vm); 
                    break;
                }
            case OP_GLOBAL_GET:
                {
                    Value name = read_constant();
                    Value value;
                    int present = mapGet(&vm->globals, name, &value);
                    if (!present) {
                        runtimeError(vm, "cannot get value of undefined global variable");
                        return RUNTIME_ERROR;
                    } else {
                        push(vm, value);
                    }
                    break;
                }
            case OP_GLOBAL_GET_LONG:
                {
                    Value name = read_constant_long();
                    Value value;
                    int present = mapGet(&vm->globals, name, &value);
                    if (!present) {
                        runtimeError(vm, "cannot get value of undefined global variable");
                        return RUNTIME_ERROR;
                    } else {
                        push(vm, value);
                    }
                    break;
                }
            case OP_GLOBAL_SET:
                {
                    Value name = read_constant();
                    Value value;
                    if (!mapGet(&vm->globals, name, &value)) {
                        runtimeError(vm, "cannot assign undefined global variable");
                        return RUNTIME_ERROR;
                    } else {
                        mapPut(vm->collector, &vm->globals, name, peek(vm, 0));
                    }
                    break;
                }
            case OP_GLOBAL_SET_LONG:
                {
                    Value name = read_constant_long();
                    Value value;
                    if (!mapGet(&vm->globals, name, &value)) {
                        runtimeError(vm, "cannot assign undefined global variable");
                        return RUNTIME_ERROR;
                    } else {
                        mapPut(vm->collector, &vm->globals, name, peek(vm, 0));
                    }
                    break;
                }
            case OP_LOCAL_GET:
                {
                    uint8_t argument = read_byte();
                    push(vm, currentFrame->localStack[argument]);
                    break;
                }
            case OP_LOCAL_GET_LONG:
                {
                    uint16_t argument = read_long();
                    push(vm, currentFrame->localStack[argument]);
                    break;
                }
            case OP_LOCAL_SET:
                {
                    uint8_t argument = read_byte();
                    currentFrame->localStack[argument] = peek(vm, 0);
                    break;
                }
            case OP_LOCAL_SET_LONG:
                {
                    uint8_t argument = read_long();
                    currentFrame->localStack[argument] = peek(vm, 0);
                    break;
                }
            case OP_JUMP_IF_FALSE:
                {
                    uint8_t* oldpc = currentFrame->pc - 1;
                    uint16_t argument = read_long();
                    if (!isTruthy(peek(vm, 0)))
                        currentFrame->pc = oldpc + argument;
                    break;
                }
            case OP_JUMP_IF_TRUE:
                {
                    uint8_t* oldpc = currentFrame->pc - 1;
                    uint16_t argument = read_long();
                    if (isTruthy(peek(vm, 0)))
                        currentFrame->pc = oldpc + argument;
                    break;
                }
            case OP_JUMP:
                {
                    uint8_t* oldpc = currentFrame->pc - 1;
                    uint16_t argument = read_long();
                    currentFrame->pc = oldpc + argument;
                    break;
                }
            case OP_JUMP_BACK:
                {
                    uint8_t* oldpc = currentFrame->pc - 1;
                    uint16_t argument = read_long();
                    currentFrame->pc = oldpc - argument;
                    break;
                }
            case OP_XOR:
                {
                    Value b = peek(vm, 0);
                    Value a = peek(vm, 1);
                    int bb = isTruthy(b);
                    int ba = isTruthy(a);
                    pop(vm);
                    pop(vm);
                    push(vm, to_vbool(!(ba == bb)));
                    break;
                }
            case OP_NEGATE:
                {
                    if (!is_number(peek(vm, 0))) {
                        runtimeError(vm, "only numbers can be negated");
                        return RUNTIME_ERROR;
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
                        return RUNTIME_ERROR;
                    }
                    if (as_cnumber(peek(vm, 0)) == 0) {
                        runtimeError(vm, "cannot divide by zero (/ 0)");
                        return RUNTIME_ERROR;
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
                        return RUNTIME_ERROR;
                    }
                    if (as_cnumber(peek(vm, 0)) == 0) {
                        runtimeError(vm, "cannot divide by 0 (% 0)");
                        return RUNTIME_ERROR;
                    }
                    if (!valuesIntegers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "only integer allowed when using %");
                        return RUNTIME_ERROR;
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
                        return RUNTIME_ERROR;
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
                        return RUNTIME_ERROR;
                    }
                    Value b = pop(vm);
                    Value a = pop(vm);
                    push(vm, concatenate(vm->collector, a, b));
                    break;
                }
            case OP_PRINT:
                {
                    Value val = peek(vm, 0);
                    printValue(val);
                    printf("\n");
                    pop(vm);
                    break;
                }
            default:
                {
                    runtimeError(vm, "unknown instruction");
                    return RUNTIME_ERROR;
                    break;
                }
        }
    }
#undef read_byte
#undef read_constant
#undef read_constant_long
}

int vmExecute(VM* vm, Collector* collector, ObjFunction* function) {
    initVM(vm);
    CallFrame* initialFrame = &vm->frames[0];
    initialFrame->function = function;
    initialFrame->pc = function->chunk->code;
    initialFrame->localStack = vm->stack;

    vm->collector = collector;
    int result = vmRun(vm);
    freeVM(vm);
    return result;
}

void freeVM(VM* vm) {
#ifdef TRACE_INTERNED
    printf("INTERNED:\n");
    printMap(&vm->collector->interned);
    printf("\n");
#endif
#ifdef TRACE_GLOBALS
    printf("GLOBALS:\n");
    printMap(&vm->globals);
    printf("\n");
#endif
    freeCollector(vm->collector);
    freeMap(NULL, &vm->globals);
}
