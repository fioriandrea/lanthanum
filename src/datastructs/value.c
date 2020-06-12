#include "value.h"
#include "../memory.h"
#include "../util.h"
#include "bytecode.h"
#include "../debug/debug_switches.h"

#ifdef TRACE_GC
static inline char* string_type(ObjType type) {
#define type_case(type) case type: return #type;
    switch (type) {
        type_case(OBJ_STRING)
            type_case(OBJ_FUNCTION)
            type_case(OBJ_CLOSURE)
            type_case(OBJ_UPVALUE)
            type_case(OBJ_ARRAY)
            type_case(OBJ_DICT)
            type_case(OBJ_ERROR)
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
        dumpObj(o);
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

ObjString* copyNoLengthString(Collector* collector, char* chars) {
    return copyString(collector, chars, strlen(chars));
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
    function->bytecode = allocate_pointer(collector, Bytecode, sizeof(Bytecode));
    popSafe(collector);
    initBytecode(function->bytecode);
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

ObjArray* newArray(Collector* collector) {
    ValueArray* values = allocate_pointer(collector, ValueArray, sizeof(ValueArray));
    initValueArray(values);
    ObjArray* array = allocate_obj(collector, ObjArray, OBJ_ARRAY);
    array->values = values;
    return array;
}

ObjDict* newDict(Collector* collector) {
    HashMap* map = allocate_pointer(collector, HashMap, sizeof(HashMap));
    initMap(map);
    ObjDict* dict = allocate_obj(collector, ObjDict, OBJ_DICT);
    dict->map = map;
    return dict;
}

ObjError* newError(Collector* collector, ObjString* message) {
    ObjError* error = allocate_obj(collector, ObjError, OBJ_ERROR);
    error->message = message;
    error->payload = NULL;   
    return error;
}

void closeUpvalue(ObjUpvalue* upvalue) {
    upvalue->closed = allocate_pointer(NULL, Value, sizeof(Value));
    *upvalue->closed = *upvalue->value;
    upvalue->value = upvalue->closed;
}

void freeObject(Collector* collector, Obj* object) {
#ifdef TRACE_GC
    printf("(pointer %p) free %s object type [", (void*) object, string_type(object->type));
    dumpObj(object);
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
                freeBytecode(collector, function->bytecode);
                free_pointer(collector, function->bytecode, sizeof(Bytecode));
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
        case OBJ_ERROR:
            {   
                ObjError* error = (ObjError*) object;
                free_pointer(collector, error, sizeof(ObjError));
                break;
            }
        case OBJ_ARRAY:
            {
                ObjArray* array = (ObjArray*) object;
                freeValueArray(collector, array->values);
                free_pointer(collector, array->values, sizeof(ValueArray));
                free_pointer(collector, array, sizeof(ObjArray));
                break;                    
            }
        case OBJ_DICT:
            {
                ObjDict* dict = (ObjDict*) object;
                freeMap(collector, dict->map);
                free_pointer(collector, dict->map, sizeof(HashMap));
                free_pointer(collector, dict, sizeof(ObjDict));
                break;                    
            }
    }
}

void blackenObject(Collector* collector, Obj* obj) {
#ifdef TRACE_GC                     
    printf("(pointer %p) blacken ", (void*) obj); 
    dumpObj(obj);          
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
                markBytecode(collector, fn->bytecode);
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
        case OBJ_ERROR:
            {
                ObjError* err = (ObjError*) obj;
                markObject(collector, (Obj*) err->message);
                if (err->payload != NULL) {
                    markValue(collector, *err->payload);
                }
                break;
            }
        case OBJ_ARRAY:
            {   
                ObjArray* array = (ObjArray*) obj;
                for (int i = 0; i < array->values->count; i++) {
                    markValue(collector, array->values->values[i]);
                }
                break;
            }
        case OBJ_DICT:
            {   
                ObjDict* dict = (ObjDict*) obj;
                markMap(collector, dict->map);
                break;
            }
    }
}

void markObject(Collector* collector, Obj* obj) {
    if (obj == NULL)
        return;
#ifdef TRACE_GC
    printf("(pointer %p) mark ", (void*) obj);
    dumpObj(obj);      
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


int isObjType(Value value, ObjType type) {
    return is_obj(value) && as_obj(value)->type == type;
}

void initValueArray(ValueArray* valarray) {
    valarray->count = 0;
    valarray->capacity = 0;
    valarray->values = NULL;
}

int writeValueArray(Collector* collector, ValueArray* valarray, Value value) {
    if (valarray->count + 1 >= valarray->capacity) {
        int newcap = compute_capacity(valarray->capacity);
        valarray->values = grow_array(collector, Value, valarray->values, valarray->capacity, newcap);
        valarray->capacity = newcap; 
    }     
    valarray->values[valarray->count++] = value;
    return valarray->count - 1;
}

void freeValueArray(Collector* collector, ValueArray* valarray) {
    free_array(collector, uint8_t, valarray->values, valarray->capacity);
    initValueArray(valarray);
}

uint32_t hashValue(Value value) {
    switch (value.type) {
        case VALUE_NIHL: return hash_nihl;
        case VALUE_BOOL: return hash_bool(value);
        case VALUE_NUMBER: return hash_number(value);
        case VALUE_OBJ: return as_obj(value)->hash;
    }
}

void markValue(Collector* collector, Value value) {
    if (!is_obj(value))
        return;
    markObject(collector, as_obj(value));
}

void markValueArray(Collector* collector, ValueArray* values) {
    for (int i = 0; i < values->count; i++)
        markValue(collector, values->values[i]);
}

