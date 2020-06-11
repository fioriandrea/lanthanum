#include <stdio.h>
#include <string.h>

#include "value.h"
#include "../util.h"
#include "../memory.h"
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

void arrayPush(Collector* collector, ObjArray* array, Value* value) {
    pushSafe(collector, to_vobj(array));
    writeValueArray(collector, array->values, *value);
    popSafe(collector);
}

int dictPut(Collector* collector, ObjDict* dict, Value* key, Value* value) {
    pushSafe(collector, to_vobj(dict));
    int res = mapPut(collector, dict->map, *key, *value);
    popSafe(collector);
    return res;
}

int dictGet(ObjDict* dict, Value* key, Value* result) {
    return mapGet(dict->map, *key, result);
}

static ObjArray* concatenateArrays(Collector* collector, ObjArray* a, ObjArray* b) {
    ObjArray* newArr = newArray(collector);
    for (int i = 0; i < a->values->count; i++) {
        arrayPush(collector, newArr, &a->values->values[i]);
    }
    for (int i = 0; i < b->values->count; i++) {
        arrayPush(collector, newArr, &b->values->values[i]);
    }
    return newArr;
}

static ObjString* concatenateStrings(Collector* collector, ObjString* sa, ObjString* sb) {
    int length = sa->length + sb->length;
    char* chars = allocate_block(collector, char, length);
    memcpy(chars, sa->chars, sa->length);
    memcpy(chars + sa->length, sb->chars, sb->length);
    chars[length] = '\0'; 
    return takeString(collector, chars, length);
}

ObjString* concatenateCharArrays(Collector* collector, char* first, ...) {
    va_list args;                                     
    va_start(args, first);
    ObjString* result = vconcatenateCharArrays(collector, first, args); 
    va_end(args);
    return result;
}

ObjString* vconcatenateCharArrays(Collector* collector, char* first, va_list rest) {
    ObjString* result = copyString(collector, first, strlen(first)); 
    char* partial = NULL;
    for (;;) {
        partial = va_arg(rest, char*);
        if (partial == NULL)
            break;
        ObjString* sa = result;
        pushSafe(collector, to_vobj(sa));
        ObjString* sb = copyString(collector, partial, strlen(partial));
        pushSafe(collector, to_vobj(sb));
        result = concatenateStrings(collector, sa, sb);
        popSafe(collector);
        popSafe(collector);
    }
    return result;
}

Obj* concatenateObjects(Collector* collector, Obj* a, Obj* b) {
    if (a->type != b->type) {
        return (Obj*) newError(collector, "cannot concatenate objects of different types", NULL);
    }
    switch (a->type) {
        case OBJ_STRING:
            return (Obj*) concatenateStrings(collector, (ObjString*) a, (ObjString*) b);
        case OBJ_ARRAY:
            return (Obj*) concatenateArrays(collector, (ObjArray*) a, (ObjArray*) b);
        default:
            return (Obj*) newError(collector, "cannot concatenate objects that are not strings or arrays", NULL);
    }
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

ObjError* newError(Collector* collector, char* first, ...) {
    va_list args;                                     
    va_start(args, first);
    ObjString* message = vconcatenateCharArrays(collector, first, args); 
    va_end(args);

    pushSafe(collector, to_vobj(message));
    ObjError* error = allocate_obj(collector, ObjError, OBJ_ERROR);
    popSafe(collector);
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
    printf("(pointer %p) free %s object type [", (void*)object, string_type(object->type));
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

static int indexGetArray(Collector* collector, ObjArray* array, Value* index, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newError(collector, "invalid index for array", NULL));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    int count = array->values->count;
    if (cindex < 0 || cindex >= count) {
        *result = to_vobj(newError(collector, "array index out of bounds", NULL));
        return 0;
    }
    *result = array->values->values[cindex];
    return 1;
}

static int indexSetArray(Collector* collector, ObjArray* array, Value* index, Value* value, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newError(collector, "invalid index for array", NULL));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    int count = array->values->count;
    if (cindex < 0 || cindex >= count) {
        *result = to_vobj(newError(collector, "array index out of bounds", NULL));
        return 0;
    }
    array->values->values[cindex] = *value;
    return 1;
}

static int indexGetString(Collector* collector, ObjString* string, Value* index, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newError(collector, "invalid index for string", NULL));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    if (cindex < 0 || cindex >= string->length) {
        *result = to_vobj(newError(collector, "string index out of bounds", NULL));
        return 0;
    }
    *result = to_vobj(copyString(collector, string->chars + cindex, 1));
    return 1;
}

void indexGetObject(Collector* collector, Obj* array, Value* index, Value* result) {
    switch (array->type) {
        case OBJ_STRING:
            {
                indexGetString(collector, (ObjString*) array, index, result);
                break;
            }
        case OBJ_ARRAY:
            {
                indexGetArray(collector, (ObjArray*) array, index, result);
                break;
            }
        case OBJ_DICT:
            {
                dictGet((ObjDict*) array, index, result);
                break;;
            }
        default:
            {
                *result = to_vobj(newError(collector, "object not indexable", NULL));
                break;
            }
    }
}

void indexSetObject(Collector* collector, Obj* array, Value* index, Value* value, Value* result) {
    switch (array->type) {
        case OBJ_ARRAY:
            {
                indexSetArray(collector, (ObjArray*) array, index, value, result);
                break;
            }
        case OBJ_DICT:
            {
                dictPut(collector, (ObjDict*) array, index, value);
                *result = *value;
                break;
            }
        default:
            {
                *result = to_vobj(newError(collector, "object index not assignable", NULL));
                break;
            }
    }
}

static ObjString* strOrSelf(Collector* collector, Obj* container, Value value) {
    if (is_obj(value) && as_obj(value) == container)
        return concatenateCharArrays(collector, "<self>", NULL);
    else
        return valueToString(collector, value);
}

ObjString* objectToString(Collector* collector, Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            return ((ObjString*) obj);
        case OBJ_FUNCTION:
            {
                ObjFunction* function = (ObjFunction*) obj;
                if (function->name == NULL)
                    return concatenateCharArrays(collector, "<init>", NULL);
                else
                    return concatenateCharArrays(collector, "<", function->name->chars, " function>", NULL);
            }
        case OBJ_CLOSURE:
            {
                ObjClosure* closure = (ObjClosure*) obj;
                return objectToString(collector, (Obj*) closure->function);
            }
        case OBJ_UPVALUE:
            {
                ObjUpvalue* upvalue = (ObjUpvalue*) obj;
                return concatenateCharArrays(collector, "upvalue ", 
                        valueToCharArray(collector, *upvalue->value), NULL);
            }
        case OBJ_ERROR:
            {
                ObjError* error = (ObjError*) obj;
                ObjString* result = error->message;
                if (error->payload != NULL) {
                    return concatenateCharArrays(collector, " (", 
                            valueToCharArray(collector, *error->payload), ")", NULL);
                }
                return result;
            }
        case OBJ_ARRAY:
            {
                ObjArray* array = (ObjArray*) obj;
                ObjString* result = concatenateCharArrays(collector, "[", NULL);
                for (int i = 0; i < array->values->count - 1; i++) {
                    Value val = array->values->values[i];
                    result = concatenateCharArrays(collector, result->chars,
                            strOrSelf(collector, obj, val)->chars, ",", NULL);
                }
                if (array->values->count > 0) {
                    int index = array->values->count - 1;
                    Value val = array->values->values[index];
                    result = concatenateCharArrays(collector, result->chars,
                            strOrSelf(collector, obj, val)->chars, NULL);
                }
                result = concatenateCharArrays(collector, result->chars, "]", NULL);
                return result;
            }
        case OBJ_DICT:
            {
                ObjDict* dict = (ObjDict*) obj;
                ObjString* result = concatenateCharArrays(collector, "{", NULL);
                HashMap* map = dict->map;
                for (int i = 0; i < map->capacity; i++) {
                    Entry* entry = map->entries[i];
                    while (entry != NULL) {
                        result = concatenateCharArrays(collector, result->chars, 
                                strOrSelf(collector, obj, entry->key)->chars, " => ",
                                strOrSelf(collector, obj, entry->value)->chars, ",", NULL);
                        entry = entry->next;
                    }
                }
                result = concatenateCharArrays(collector, result->chars, "}", NULL);
                return result;
            }
    }
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

int isTruthy(Value val) {
    return !(is_nihl(val) || (is_bool(val) && !as_cbool(val)) ||
            (is_number(val) && as_cnumber(val) == 0));
}

int valueInteger(Value value) {
    if (!is_number(value))
        return 0;
    return is_integer(as_cnumber(value));
}

int valuesIntegers(Value a, Value b) {
    return valueInteger(a) && valueInteger(b);
}

int valuesEqual(Value a, Value b) {
    if (a.type != b.type)
        return 0;
    switch (a.type) {
        case VALUE_NIHL: return 1;
        case VALUE_NUMBER: return as_cnumber(a) == as_cnumber(b);
        case VALUE_BOOL: return as_cbool(a) == as_cbool(b);
        case VALUE_OBJ: return as_obj(a) == as_obj(b);
    }
}

int valuesConcatenable(Value a, Value b) {
    return is_string(a) && is_string(b);
}

int valuesNumbers(Value a, Value b) {
    return is_number(a) && is_number(b);
}

Value concatenate(Collector* collector, Value a, Value b) {
    if (!is_obj(a) || !is_obj(b)) {
        return to_vobj(newError(collector, "cannot concatenate non objects", NULL));
    }
    return to_vobj(concatenateObjects(collector, as_obj(a), as_obj(b)));
}

void printValue(Collector* collector, Value val) {
    printf("%s", valueToCharArray(collector, val)); 
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

Value indexGetValue(Collector* collector, Value arrayLike, Value index) {
    if (!is_obj(arrayLike)) {
        return to_vobj(newError(collector, "value not indexable", NULL));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result = to_vnihl();
    indexGetObject(collector, arrayObj, &index, &result);
    return result;
}

Value indexSetValue(Collector* collector, Value arrayLike, Value index, Value value) {
    if (!is_obj(arrayLike)) {
        return to_vobj(newError(collector, "value not indexable", NULL));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result;
    indexSetObject(collector, arrayObj, &index, &value, &result);
    return result;
}

ObjString* valueToString(Collector* collector, Value value) {
    switch (value.type) {
        case VALUE_BOOL:
            return concatenateCharArrays(collector, as_cbool(value) ? "true" : "false", NULL);
        case VALUE_NUMBER:
            {
#define MAX_DOUBLE_STR_LEN 80
                char numstr[MAX_DOUBLE_STR_LEN];
                sprintf(numstr, "%g", as_cnumber(value));
                return concatenateCharArrays(collector, numstr, NULL);
#undef MAX_DOUBLE_STR_LEN
            }
        case VALUE_NIHL:
            return concatenateCharArrays(collector, "nihl", NULL);
        case VALUE_OBJ:
            return objectToString(collector, as_obj(value));
    }
}

char* valueToCharArray(Collector* collector, Value value) {
    return valueToString(collector, value)->chars;
}
