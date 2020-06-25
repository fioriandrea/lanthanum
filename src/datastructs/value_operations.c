#include <stdio.h>
#include <string.h>

#include "../util.h"
#include "value_operations.h"
#include "value.h"
#include "../memory.h"

// GC INVARIANT: PARAMETERS PASSED ARE ALREADY ON THE STACK (exceptions are *Safe functions)

int indexGetArray(Collector* collector, ObjArray* array, Value* index, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newErrorFromCharArray(collector, "invalid index for array"));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    int count = array->values->count;
    if (cindex < 0 || cindex >= count) {
        *result = to_vobj(newErrorFromCharArray(collector, "array index out of bounds"));
        return 0;
    }
    *result = array->values->values[cindex];
    return 1;
}

int indexSetArray(Collector* collector, ObjArray* array, Value* index, Value* value, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newErrorFromCharArray(collector, "invalid index for array"));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    int count = array->values->count;
    if (cindex < 0 || cindex >= count) {
        *result = to_vobj(newErrorFromCharArray(collector, "array index out of bounds"));
        return 0;
    }
    array->values->values[cindex] = *value;
    return 1;
}

int indexGetString(Collector* collector, ObjString* string, Value* index, Value* result) {
    if (!valueInteger(*index)) {
        *result = to_vobj(newErrorFromCharArray(collector, "invalid index for string"));
        return 0;
    }
    int cindex = (int) as_cnumber(*index);
    if (cindex < 0 || cindex >= string->length) {
        *result = to_vobj(newErrorFromCharArray(collector, "string index out of bounds"));
        return 0;
    }
    *result = to_vobj(copyString(collector, string->chars + cindex, 1));
    return 1;
}

void indexGetObject(Collector* collector, Obj* array, Value* index, Value* result) {
    switch (array->type) {
        case OBJ_STRING:
                indexGetString(collector, (ObjString*) array, index, result);
                break;
        case OBJ_ARRAY:
                indexGetArray(collector, (ObjArray*) array, index, result);
                break;
        case OBJ_DICT:
                indexGetDict((ObjDict*) array, index, result);
                break;
        default:
                *result = to_vobj(newErrorFromCharArray(collector, "object not indexable"));
                break;
    }
}

void indexSetObject(Collector* collector, Obj* array, Value* index, Value* value, Value* result) {
    switch (array->type) {
        case OBJ_ARRAY:
                indexSetArray(collector, (ObjArray*) array, index, value, result);
                break;
        case OBJ_DICT:
                indexSetDict(collector, (ObjDict*) array, index, value);
                *result = *value;
                break;
        default:
                *result = to_vobj(newErrorFromCharArray(collector, "object index not assignable"));
                break;
    }
}

static ObjArray* concatenateArrays(Collector* collector, ObjArray* a, ObjArray* b) {
    ObjArray* newArr = newArray(collector);
    pushSafeObj(collector, newArr);
    for (int i = 0; i < a->values->count; i++) {
        arrayPush(collector, newArr, a->values->values[i]);
    }
    for (int i = 0; i < b->values->count; i++) {
        arrayPush(collector, newArr, b->values->values[i]);
    }
    popSafe(collector);
    return newArr;
}

ObjString* concatenateStrings(Collector* collector, ObjString* sa, ObjString* sb) {
    int length = sa->length + sb->length;
    if (length == 0)
        return copyString(collector, "", 0);
    
    char* chars = allocate_block(collector, char, length + 1);
    memcpy(chars, sa->chars, sa->length);
    memcpy(chars + sa->length, sb->chars, sb->length);
    chars[length] = '\0'; 
    ObjString* result = takeString(collector, chars, length);
    return result;
}

ObjString* concatenateStringsSafe(Collector* collector, ObjString* sa, ObjString* sb) {
    pushSafeObj(collector, sa);
    pushSafeObj(collector, sb);
    ObjString* result = concatenateStrings(collector, sa, sb);
    popSafe(collector);
    popSafe(collector);
    return result;
}

ObjString* concatenateCharArrays(Collector* collector, char* s1, char* s2) {
    ObjString* first = copyNoLengthString(collector, s1);
    return concatenateStringAndCharArraySafe(collector, first, s2);
}

ObjString* concatenateStringAndCharArray(Collector* collector, ObjString* str, char* carr) {
    ObjString* second = copyString(collector, carr, strlen(carr));
    return concatenateStringsSafe(collector, str, second);
}

ObjString* concatenateStringAndCharArraySafe(Collector* collector, ObjString* str, char* carr) {
    pushSafeObj(collector, str);
    ObjString* second = copyNoLengthString(collector, carr);
    popSafe(collector);
    ObjString* result = concatenateStringsSafe(collector, str, second);
    return result;
}

ObjString* concatenateMultipleCharArrays(Collector* collector, char* first, ...) {
    va_list args;                                     
    va_start(args, first);
    ObjString* result = vconcatenateMultipleCharArrays(collector, first, args); 
    va_end(args);
    return result;
}

ObjString* vconcatenateMultipleCharArrays(Collector* collector, char* first, va_list rest) {
    ObjString* result = copyString(collector, first, strlen(first)); 
    char* partial = NULL;
    for (;;) {
        partial = va_arg(rest, char*);
        if (partial == NULL)
            break;
        ObjString* sa = result;
        pushSafeObj(collector, sa);
        ObjString* sb = copyString(collector, partial, strlen(partial));
        pushSafeObj(collector, sb);
        result = concatenateStrings(collector, sa, sb);
        popSafe(collector);
        popSafe(collector);
    }
    return result;
}

static ObjString* strOrSelf(Collector* collector, Obj* container, Value value) {
    ObjString* result;
    if (is_obj(value) && as_obj(value) == container)
        result = copyNoLengthString(collector, "<self>");
    else
        result = valueToString(collector, value);
    return result;
}

ObjString* objectToString(Collector* collector, Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            return ((ObjString*) obj);
        case OBJ_NATIVE_FUNCTION:
            {
                ObjNativeFunction* native = (ObjNativeFunction*) obj;
                ObjString* result = copyNoLengthString(collector, "<");
                result = concatenateStringsSafe(collector, result, native->name);
                result = concatenateStringAndCharArraySafe(collector, result, " native function>"); 
                return result;
            }
        case OBJ_FUNCTION:
            {
                ObjFunction* function = (ObjFunction*) obj;
                if (function->name == NULL) {
                    return copyNoLengthString(collector, "<init>");
                } else {
                    ObjString* result = copyNoLengthString(collector, "<");
                    result = concatenateStringsSafe(collector, result, function->name);
                    result = concatenateStringAndCharArraySafe(collector, result, " function>"); 
                    return result;
                }
            }
        case OBJ_CLOSURE:
            {
                ObjClosure* closure = (ObjClosure*) obj;
                return objectToString(collector, (Obj*) closure->function);
            }
        case OBJ_UPVALUE:
            {
                ObjUpvalue* upvalue = (ObjUpvalue*) obj;
                ObjString* result = copyNoLengthString(collector, "upvalue ");
                pushSafeObj(collector, result);
                result = concatenateStringsSafe(collector, result, 
                        valueToString(collector, *upvalue->value));
                popSafe(collector);
                return result;
            }
        case OBJ_ERROR:
            {
                ObjError* error = (ObjError*) obj;
                ObjString* result = error->message;
                if (error->payload != NULL) {
                    result = concatenateStringAndCharArraySafe(collector, result, " (");
                    pushSafeObj(collector, result);
                    result = concatenateStringsSafe(collector, result,
                            valueToString(collector, *error->payload));
                    popSafe(collector);
                    result = concatenateStringAndCharArray(collector, result, ")");
                }
                return result;
            }
        case OBJ_ARRAY:
            {
                ObjArray* array = (ObjArray*) obj;
                ObjString* result = copyNoLengthString(collector, "[");
                for (int i = 0; i < array->values->count - 1; i++) {
                    Value val = array->values->values[i];
                    pushSafeObj(collector, result);
                    result = concatenateStringsSafe(collector, result, strOrSelf(collector, obj, val));
                    popSafe(collector);
                    result = concatenateStringAndCharArraySafe(collector, result, ",");
                }
                if (array->values->count > 0) {
                    int index = array->values->count - 1;
                    Value val = array->values->values[index];
                    pushSafeObj(collector, result);
                    result = concatenateStringsSafe(collector, result, strOrSelf(collector, obj, val));
                    popSafe(collector);
                }
                result = concatenateStringAndCharArraySafe(collector, result, "]");
                return result;
            }
        case OBJ_DICT:
            {
                ObjDict* dict = (ObjDict*) obj;
                ObjString* result = copyNoLengthString(collector, "{");
                HashMap* map = dict->map;
                for (int i = 0; i < map->capacity; i++) {
                    Entry* entry = map->entries[i];
                    while (entry != NULL) {
                        pushSafeObj(collector, result);
                        result = concatenateStringsSafe(collector, result, 
                                strOrSelf(collector, obj, entry->key));
                        popSafe(collector);
                        result = concatenateStringAndCharArraySafe(collector, result, " => ");
                        pushSafeObj(collector, result);
                        result = concatenateStringsSafe(collector, result, 
                                strOrSelf(collector, obj, entry->value));
                        popSafe(collector);
                        result = concatenateStringAndCharArraySafe(collector, result, ",");
                        entry = entry->next;
                    }
                }
                result = concatenateStringAndCharArraySafe(collector, result, "}");
                return result;
            }
    }
}

Obj* concatenateObjects(Collector* collector, Obj* a, Obj* b) {
    if (a->type != b->type) {
        return (Obj*) newErrorFromCharArray(collector, "cannot concatenate objects of different types");
    }
    switch (a->type) {
        case OBJ_STRING:
            return (Obj*) concatenateStrings(collector, (ObjString*) a, (ObjString*) b);
        case OBJ_ARRAY:
            return (Obj*) concatenateArrays(collector, (ObjArray*) a, (ObjArray*) b);
        default:
            return (Obj*) newErrorFromCharArray(collector, 
                    "cannot concatenate objects that are not strings or arrays");
    }
}

ObjArray* pairList(Collector* collector, Obj* arrayLike) {
    ObjArray* result = newArray(collector);
    pushSafeObj(collector, result);
    switch (arrayLike->type) {
        case OBJ_STRING:
            {
                ObjString* str = (ObjString*) arrayLike;
                for (int i = 0; i < str->length; i++) {
                    ObjArray* pair = newArray(collector);
                    pushSafeObj(collector, pair);
                    arrayPush(collector, pair, to_vnumber(i));
                    arrayPush(collector, pair, to_vobj(copyString(collector, str->chars + i, 1)));
                    arrayPush(collector, result, to_vobj(pair));
                    popSafe(collector);
                }
                break;
            }
        case OBJ_ARRAY:
            {
                ObjArray* arr = (ObjArray*) arrayLike;
                for (int i = 0; i < arr->values->count; i++) {
                    ObjArray* pair = newArray(collector);
                    pushSafeObj(collector, pair);
                    arrayPush(collector, pair, to_vnumber(i));
                    arrayPush(collector, pair, arr->values->values[i]);
                    arrayPush(collector, result, to_vobj(pair));
                    popSafe(collector);
                }
                break;
            }
        case OBJ_DICT:
            {
                ObjDict* dict = (ObjDict*) arrayLike;
                HashMap* map = dict->map;
                for (int i = 0; i < map->capacity; i++) {
                    Entry* entry = map->entries[i];
                    while (entry != NULL) {
                        ObjArray* pair = newArray(collector);
                        pushSafeObj(collector, pair);
                        arrayPush(collector, pair, entry->key);
                        arrayPush(collector, pair, entry->value);
                        arrayPush(collector, result, to_vobj(pair));
                        popSafe(collector);
                        entry = entry->next;
                    }
                }
                break;
            }
    }
    popSafe(collector);
    return result;
}

void arrayPush(Collector* collector, ObjArray* array, Value value) {
    writeValueArray(collector, array->values, value);
}

int indexSetDict(Collector* collector, ObjDict* dict, Value* key, Value* value) {
    int res = mapPut(collector, dict->map, *key, *value);
    return res;
}

int indexGetDict(ObjDict* dict, Value* key, Value* result) {
    return mapGet(dict->map, *key, result);
}

int valueIndexable(Value val) {
    return is_array(val) || is_string(val) || is_dict(val);
}

Value indexGetValue(Collector* collector, Value arrayLike, Value index) {
    if (!valueIndexable(arrayLike)) {
        return to_vobj(newErrorFromCharArray(collector, "value not indexable"));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result = to_vnihl();
    indexGetObject(collector, arrayObj, &index, &result);
    return result;
}

Value indexSetValue(Collector* collector, Value arrayLike, Value index, Value value) {
    if (!valueIndexable(arrayLike)) {
        return to_vobj(newErrorFromCharArray(collector, "value not indexable"));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result;
    indexSetObject(collector, arrayObj, &index, &value, &result);
    return result;
}

ObjString* valueToString(Collector* collector, Value value) {
    switch (value.type) {
        case VALUE_BOOL:
            return copyNoLengthString(collector, as_cbool(value) ? "true" : "false");
        case VALUE_NUMBER:
            {
#define MAX_DOUBLE_STR_LEN 80
                char numstr[MAX_DOUBLE_STR_LEN];
                sprintf(numstr, "%g", as_cnumber(value));
                return copyNoLengthString(collector, numstr);
#undef MAX_DOUBLE_STR_LEN
            }
        case VALUE_NIHL:
            return copyNoLengthString(collector, "nihl");
        case VALUE_OBJ:
            return objectToString(collector, as_obj(value));
    }
}

void printValue(Collector* collector, Value val) {
    printf("%s", valueToString(collector, val)->chars); 
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

int isCallable(Value value) {
    return is_closure(value) || is_native(value);
}

Value concatenate(Collector* collector, Value a, Value b) {
    if (!is_obj(a) || !is_obj(b)) {
        return to_vobj(newErrorFromCharArray(collector, "cannot concatenate non objects"));
    }
    return to_vobj(concatenateObjects(collector, as_obj(a), as_obj(b)));
}

int arrayLikeLength(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            return ((ObjString*) obj)->length;
        case OBJ_ARRAY:
            return ((ObjArray*) obj)->values->count;
    }
    return -1;
}
