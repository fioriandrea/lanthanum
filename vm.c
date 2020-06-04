#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "vm.h"
#include "memory.h"
#include "util.h"
#include "./datastructs/value.h"
#include "./compilation_pipeline/compiler.h"
#include "./debug/debug_switches.h"

#define RUNTIME_ERROR 0
#define RUNTIME_OK 1

static void resetStack(VM* vm) {
    vm->sp = vm->stack;
}

void initVM(VM* vm) {
    vm->fp = 0;
    resetStack(vm);
    initMap(&vm->globals);
    vm->openUpvalues = NULL;
    vm->collector = NULL;
}

void vmPush(VM* vm, Value val) {
    *vm->sp = val;
    vm->sp++;
}

Value vmPop(VM* vm) {
    vm->sp--;
    return *vm->sp;
}

static void runtimeError(VM* vm, char* format, ...) {
    int instruction = vm->frames[vm->fp - 1].pc - vm->frames[vm->fp - 1].closure->function->chunk->code - 1; 
    int line = lineArrayGet(&vm->frames[vm->fp - 1].closure->function->chunk->lines, instruction);         

    va_list args;                                    
    va_start(args, format);                          
    fprintf(stderr, "runtime error [line %d] in program: ", line);  
    vfprintf(stderr, format, args);                  
    va_end(args);                                    
    fputs("\n", stderr);
    resetStack(vm);                                    
}    

static Value peek(VM* vm, int depth) {
    return vm->sp[-(depth + 1)];
}

static void closeOnStackUpvalue(VM* vm, Value* value) {
    // todo: this can be optimized
    ObjUpvalue* prev = NULL;
    ObjUpvalue* current = vm->openUpvalues;
    if (current == NULL)
        return;
    if (current->value == value) {
        vm->openUpvalues = current->next;
        closeUpvalue(current);
    } else {
        prev = current;
        current = current->next;
        while (current != NULL) {
            if (current->value == value) {
                prev->next = current->next;
                closeUpvalue(current);
                break;
            }
            prev = prev->next;
            current = current->next;
        }
    }
}

static int vmRun(VM* vm) {
    vm->fp = 1;
    CallFrame* currentFrame = &vm->frames[0];
    OpCode caseCode;
#define read_byte() (*(currentFrame->pc++))
#define read_long() join_bytes(read_byte(), read_byte())
#define read_constant() (currentFrame->closure->function->chunk->constants.values[read_byte()])
#define read_constant_long() (currentFrame->closure->function->chunk->constants.values[read_long()])
#define read_long_if(oplong) (caseCode == (oplong) ? read_long() : read_byte())
#define read_constant_long_if(oplong) (caseCode == (oplong) ? read_constant_long() : read_constant())
#define binary_op(operator, destination) \
    do { \
        if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) { \
            runtimeError(vm, "operand must be numbers"); \
            return RUNTIME_ERROR; \
        } \
        double b = as_cnumber(vmPop(vm)); \
        double a = as_cnumber(vmPop(vm)); \
        vmPush(vm, destination(a operator b)); \
    } while (0)

#ifdef TRACE_EXEC
    printf("VM EXECUTION TRACE:\n");
#endif

    for (;;) {
#ifdef TRACE_EXEC
        printf("\n");
        printInstruction(currentFrame->closure->function->chunk, *currentFrame->pc, (int) (currentFrame->pc - currentFrame->closure->function->chunk->code));
        printf("stack: [");
        for (Value* start = vm->stack; start < vm->sp; start++) {
            printValue(*start);
            printf(" | ");
        }
        printf("]\n");
        printf("\n");
#endif
#ifdef TRACE_OPEN_UPVALUES
        printf("OPEN UPVALUES\n");
        for (ObjUpvalue* uv = vm->openUpvalues; uv != NULL; uv = uv->next) {
            printObj((Obj*) uv);
            printf("\n");
        }
        printf("END OPEN UPVALUES\n");
#endif
        switch ((caseCode = read_byte())) {
            case OP_RET: 
                {
                    Value retVal = vmPop(vm);
                    vm->fp--;
                    if (vm->fp == 0)
                        return RUNTIME_OK;
                    while (vm->sp > currentFrame->localStack - 1) { // -1 to skip function sitting in stack
                        // todo this can be done more efficiently
                        closeOnStackUpvalue(vm, vm->sp - 1);
                        vm->sp--;
                    }
                    currentFrame = &vm->frames[vm->fp - 1];
                    vmPush(vm, retVal);
                    break;
                }
            case OP_CALL:
                {
                    uint8_t argCount = read_byte();
                    if (argCount > (vm->sp - vm->stack)) {
                        runtimeError(vm, "too many function arguments");
                        return RUNTIME_ERROR;
                    }
                    Value closureValue = peek(vm, argCount);
                    if (!is_closure(closureValue)) {
                        runtimeError(vm, "can only call functions");
                        return RUNTIME_ERROR;
                    }
                    ObjClosure* closure = as_closure(closureValue);
                    ObjFunction* function = closure->function;
                    if (argCount != function->arity) {
                        runtimeError(vm, "expected %d arguments, got %d", function->arity, argCount);
                        return RUNTIME_ERROR;
                    }
                    if (vm->fp + 1 >= MAX_FRAMES) {             
                        runtimeError(vm, "stack overflow");             
                        return RUNTIME_ERROR;                                
                    } 
                    currentFrame = &vm->frames[vm->fp++];
                    currentFrame->closure = closure;
                    currentFrame->pc = currentFrame->closure->function->chunk->code;
                    currentFrame->localStack = vm->sp - argCount;
                    break;
                }
            case OP_CLOSURE:
            case OP_CLOSURE_LONG:
                {
                    Value funVal = read_constant_long_if(OP_CLOSURE_LONG);
                    ObjFunction* function = as_function(funVal);
                    ObjClosure* closure = newClosure(vm->collector, function);
                    vmPush(vm, to_vobj(closure));
                    for (int i = 0; i < closure->upvalueCount; i++) {
                        uint8_t ownedAbove = read_byte();
                        uint8_t index = read_byte();
                        if (ownedAbove) {
                            closure->upvalues[i] = currentFrame->closure->upvalues[index];
                        } else {
                            Value* value = currentFrame->localStack + index;
                            ObjUpvalue* current = vm->openUpvalues;
                            while (current != NULL) {
                                if (current->value == value)
                                    break;
                                current = current->next;
                            }
                            if (current == NULL) {
                                closure->upvalues[i] = newUpvalue(vm->collector, currentFrame->localStack + index);
                                closure->upvalues[i]->next = vm->openUpvalues;
                                vm->openUpvalues = closure->upvalues[i];
                            } else {
                                closure->upvalues[i] = current;
                            }
                        }
                    }
                    break;
                }
            case OP_UPVALUE_GET:
            case OP_UPVALUE_GET_LONG:
                {
                    uint16_t index = read_long_if(OP_UPVALUE_GET_LONG);
                    vmPush(vm, *currentFrame->closure->upvalues[index]->value);
                    break;
                }
            case OP_UPVALUE_SET:
            case OP_UPVALUE_SET_LONG:
                {
                    uint16_t index = read_long_if(OP_UPVALUE_SET_LONG);
                    *currentFrame->closure->upvalues[index]->value = peek(vm, 0);
                    break;
                }
            case OP_CONST: 
            case OP_CONST_LONG:
                {
                    Value constant = read_constant_long_if(OP_CONST_LONG);
                    vmPush(vm, constant);
                    break;
                }
            case OP_GLOBAL_DECL:
            case OP_GLOBAL_DECL_LONG:
                {
                    Value name = read_constant_long_if(OP_GLOBAL_DECL_LONG);
                    mapPut(vm->collector, &vm->globals, name, peek(vm, 0));
                    vmPop(vm); 
                    break;
                }
            case OP_GLOBAL_GET:
            case OP_GLOBAL_GET_LONG:
                {
                    Value name = read_constant_long_if(OP_GLOBAL_GET_LONG);
                    Value value;
                    int present = mapGet(&vm->globals, name, &value);
                    if (!present) {
                        runtimeError(vm, "cannot get value of undefined global variable");
                        return RUNTIME_ERROR;
                    } else {
                        vmPush(vm, value);
                    }
                    break;
                }
            case OP_GLOBAL_SET:
            case OP_GLOBAL_SET_LONG:
                {
                    Value name = read_constant_long_if(OP_GLOBAL_SET_LONG);
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
            case OP_LOCAL_GET_LONG:
                {
                    uint16_t argument = read_long_if(OP_LOCAL_GET_LONG);
                    vmPush(vm, currentFrame->localStack[argument]);
                    break;
                }
            case OP_LOCAL_SET:
            case OP_LOCAL_SET_LONG:
                {
                    uint16_t argument = read_long_if(OP_LOCAL_SET_LONG);
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
                    vmPop(vm);
                    vmPop(vm);
                    vmPush(vm, to_vbool(!(ba == bb)));
                    break;
                }
            case OP_NEGATE:
                {
                    if (!is_number(peek(vm, 0))) {
                        runtimeError(vm, "only numbers can be negated");
                        return RUNTIME_ERROR;
                    }
                    Value a = vmPop(vm);
                    vmPush(vm, to_vnumber(-as_cnumber(a)));
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
                    double b = as_cnumber(vmPop(vm)); 
                    double a = as_cnumber(vmPop(vm)); 
                    vmPush(vm, to_vnumber(a / b));
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

                    double b = as_cnumber(vmPop(vm)); 
                    double a = as_cnumber(vmPop(vm)); 
                    vmPush(vm, to_vnumber(((long) a) % ((long) b)));
                    break;
                }
            case OP_POW:
                {
                    if (!valuesNumbers(peek(vm, 0), peek(vm, 1))) {
                        runtimeError(vm, "operands must be numbers");
                        return RUNTIME_ERROR;
                    }
                    double b = as_cnumber(vmPop(vm)); 
                    double a = as_cnumber(vmPop(vm)); 
                    vmPush(vm, to_vnumber(pow(a, b)));
                    break;
                }
            case OP_CONST_NIHL:
                {
                    vmPush(vm, to_vnihl());
                    break;
                }
            case OP_CONST_TRUE:
                {
                    vmPush(vm, to_vbool(1));
                    break;
                }
            case OP_CONST_FALSE:
                {
                    vmPush(vm, to_vbool(0));
                    break;
                }
            case OP_NOT:
                {
                    Value val = vmPop(vm);
                    vmPush(vm, to_vbool(!isTruthy(val)));
                    break;
                }
            case OP_POP:
                {
                    vmPop(vm);
                    break;
                }
            case OP_CLOSE_UPVALUE:
                {
                    Value* value = vm->sp - 1; 
                    closeOnStackUpvalue(vm, value);
                    vmPop(vm);
                    break;
                }
            case OP_EQUAL:
                {
                    Value b = vmPop(vm);
                    Value a = vmPop(vm);
                    vmPush(vm, to_vbool(valuesEqual(a, b)));
                    break;
                }
            case OP_NOT_EQUAL:
                {
                    Value b = vmPop(vm);
                    Value a = vmPop(vm);
                    vmPush(vm, to_vbool(!valuesEqual(a, b)));
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
                    Value b = peek(vm, 0);
                    Value a = peek(vm, 1);
                    Value result = concatenate(vm->collector, a, b);
                    vmPop(vm);
                    vmPop(vm);
                    vmPush(vm, result);
                    break;
                }
            case OP_PRINT:
                {
                    Value val = peek(vm, 0);
                    printValue(val);
                    printf("\n");
                    vmPop(vm);
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
#undef read_long_if
#undef read_constant_long_if
}

int vmExecute(VM* vm, Collector* collector, ObjFunction* function) {
    initVM(vm);
    CallFrame* initialFrame = &vm->frames[0];
    initialFrame->closure = newClosure(collector, function);
    initialFrame->pc = function->chunk->code;
    initialFrame->localStack = vm->stack;
    mapPut(NULL, &vm->globals, to_vobj(initialFrame->closure), to_vnihl());

    vm->collector = collector;
    collector->vm = vm;

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
