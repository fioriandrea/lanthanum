#include <string.h>
#include <stdio.h>

#include "object.h"
#include "../util.h"
#include "../memory.h"
#include "chunk.h"
#include "value.h"
#include "../debug/debug_switches.h"

#ifdef TRACE_GC
static inline char* string_type(ObjType type) {
#define type_case(type) case type: return #type;
    switch (type) {
        type_case(OBJ_STRING)
            type_case(OBJ_FUNCTION)
            type_case(OBJ_CLOSURE)
            type_case(OBJ_UPVALUE)
    }
#undef type_case
}
#endif

#define allocate_obj(collector, type, typeenum) \
    ((type*) allocateObj(collector, typeenum, sizeof(type)))

Obj* allocateObj(Collector* collector, ObjType type, size_t size) {
#ifdef TRACE_OBJECT_LIST
    printf("OBJLIST\n");
    for (Obj* o = collector->objects; o != NULL; o = o->next) {
        printObj(o);
        printf(", ");
    }
    printf("\n");
#endif
    Obj* obj = allocate_pointer(collector, Obj, size);
    obj->type = type;
    obj->next = collector->objects;
    collector->objects = obj;
    obj->hash = hash_pointer(obj);
    obj->marked = 0;
#ifdef TRACE_GC
    printf("(pointer %p) alloc %ld bytes for %s object type\n", (void*)obj, size, string_type(type));
#endif
    return obj;
}

ObjString* copyString(Collector* collector, char* chars, int length) {
    ObjString* str;
    if ((str = containsStringDeepEqual(&collector->interned, chars, length)) != NULL) {
        return str;
    }
    char* copied = allocate_block(collector, char, length + 1);
    memcpy(copied, chars, length);
    copied[length] = '\0';
    return takeString(collector, copied, length);
}

ObjString* takeString(Collector* collector, char* chars, int length) {
    ObjString* str;
    if ((str = containsStringDeepEqual(&collector->interned, chars, length)) != NULL) {
        free_block(collector, char, chars, length);
        return str;
    }
    ObjString* string = allocate_obj(collector, ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    ((Obj*) string)->hash = hash_string(chars, length);
    pushSafe(collector, to_vobj(string));
    mapPut(collector, &collector->interned, to_vobj(string), to_vnihl());
    popSafe(collector);
    return string;
}

ObjFunction* newFunction(Collector* collector) {
    ObjFunction* function = allocate_obj(collector, ObjFunction, OBJ_FUNCTION);
    function->name = NULL;
    function->arity = 0;
    function->upvalueCount = 0;
    pushSafe(collector, to_vobj(function));
    function->chunk = allocate_pointer(collector, Chunk, sizeof(Chunk));
    popSafe(collector);
    initChunk(function->chunk);
    return function;
}

ObjClosure* newClosure(Collector* collector, ObjFunction* function) {
    ObjUpvalue** upvalues = allocate_block(collector, ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++)
        upvalues[i] = NULL;
    ObjClosure* closure = allocate_obj(collector, ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalueCount = function->upvalueCount;
    closure->upvalues = upvalues;
    return closure;
}

ObjUpvalue* newUpvalue(Collector* collector, Value* value) {
    ObjUpvalue* upvalue = allocate_obj(collector, ObjUpvalue, OBJ_UPVALUE);
    upvalue->value = value;
    upvalue->next = NULL;
    upvalue->closed = NULL;
    return upvalue;
}

void closeUpvalue(ObjUpvalue* upvalue) {
    upvalue->closed = allocate_pointer(NULL, Value, sizeof(Value));
    *upvalue->closed = *upvalue->value;
    upvalue->value = upvalue->closed;
}

void freeObject(Collector* collector, Obj* object) {
#ifdef TRACE_GC
    printf("(pointer %p) free %s object type [", (void*)object, string_type(object->type));
    printObj(object);
    printf("]\n");
#endif
    switch (object->type) {                                 
        case OBJ_STRING: 
            {                                    
                ObjString* string = (ObjString*) object;             
                free_array(collector, char, string->chars, string->length + 1);
                free_pointer(collector, object, sizeof(ObjString));                            
                break;                                              
            }       
        case OBJ_FUNCTION:
            {
                ObjFunction* function = (ObjFunction*) object;
                freeChunk(collector, function->chunk);
                free_pointer(collector, function->chunk, sizeof(Chunk));
                free_pointer(collector, function, sizeof(ObjFunction));
                break;
            }      
        case OBJ_CLOSURE:
            {
                ObjClosure* closure = (ObjClosure*) object;
                free_block(collector, ObjUpvalue*, closure->upvalues, closure->upvalueCount); 
                free_pointer(collector, closure, sizeof(ObjClosure));
                break;
            } 
        case OBJ_UPVALUE:
            {
                ObjUpvalue* upvalue = (ObjUpvalue*) object;
                if (upvalue->closed != NULL) {
                    free_pointer(collector, upvalue->closed, sizeof(Value));
                }
                free_pointer(collector, upvalue, sizeof(ObjUpvalue));
                break;
            }
    }
}

void printObj(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            printf("%s", ((ObjString*) obj)->chars);
            break;
        case OBJ_FUNCTION:
            {
                ObjFunction* function = (ObjFunction*) obj;
                if (function->name == NULL)
                    printf("<main script>");
                else
                    printf("<%s function>", function->name->chars);
                break;
            }
        case OBJ_CLOSURE:
            {
                ObjClosure* closure = (ObjClosure*) obj;
                printf("closure ");
                printObj((Obj*) closure->function);
                break;
            }
        case OBJ_UPVALUE:
            {
                ObjUpvalue* upvalue = (ObjUpvalue*) obj;
                printf("upvalue ");
                printValue(*upvalue->value);
                break;
            }
    }
}

void blackenObject(Collector* collector, Obj* obj) {
#ifdef TRACE_GC                     
    printf("(pointer %p) blacken ", (void*) obj); 
    printValue(to_vobj(obj));          
    printf("\n");                         
#endif 
    switch (obj->type) {
        case OBJ_UPVALUE:
            {
                ObjUpvalue* uv = (ObjUpvalue*) obj;
                markValue(collector, *uv->value);
                break;
            }
        case OBJ_FUNCTION:
            {
                ObjFunction* fn = (ObjFunction*) obj;
                markObject(collector, (Obj*) fn->name);
                markChunk(collector, fn->chunk);
                break;
            }
        case OBJ_CLOSURE:
            {
                ObjClosure* cl = (ObjClosure*) obj;
                markObject(collector, (Obj*) cl->function);
                for (int i = 0; i < cl->upvalueCount; i++) {
                    markObject(collector, (Obj*) cl->upvalues[i]);
                }
                break;
            }
    }
}

void markObject(Collector* collector, Obj* obj) {
    if (obj == NULL)
        return;
#ifdef TRACE_GC
    printf("(pointer %p) mark ", (void*) obj);
    printObj(obj);      
    printf("\n");   
#endif
    if (obj->marked)
        return;
    obj->marked = 1;
    if (obj->type == OBJ_STRING)
        return; 
    if (collector->worklistCapacity <= collector->worklistCount + 1) {
        collector->worklistCapacity = compute_capacity(collector->worklistCapacity);
        collector->worklist = realloc(collector->worklist, sizeof(Obj*) * (collector->worklistCapacity));
    }
    collector->worklist[collector->worklistCount++] = obj;
}
