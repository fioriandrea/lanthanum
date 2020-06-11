#include <stdio.h>
#include <string.h>

#include "../util.h"
#include "value_operations.h"
#include "value.h"
#include "../memory.h"

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
    if (length == 0)
        return copyString(collector, "", 0);
    
    pushSafe(collector, to_vobj(sa));
    pushSafe(collector, to_vobj(sb));
    char* chars = allocate_block(collector, char, length + 1);
    popSafe(collector);
    popSafe(collector);
    memcpy(chars, sa->chars, sa->length);
    memcpy(chars + sa->length, sb->chars, sb->length);
    chars[length] = '\0'; 
    ObjString* result = takeString(collector, chars, length);
    return result;
}

ObjString* concatenateCharArrays(Collector* collector, char* first, ...) {
    va_list args;                                     
    va_start(args, first);
    ObjString* result = vconcatenateCharArrays(collector, first, args); 
    va_end(args);
    return result;
}

ObjString* concatenateStringAndCharArray(Collector* collector, ObjString* str, char* carr) {
    pushSafe(collector, to_vobj(str));
    ObjString* second = copyString(collector, carr, strlen(carr));
    popSafe(collector);
    return concatenateStrings(collector, str, second);
}

ObjString* concatenateMultipleStrings(Collector* collector, ObjString* first, ...) {
    va_list args;                                     
    va_start(args, first);
    ObjString* result = vconcatenateMultipleStrings(collector, first, args); 
    va_end(args);
    return result;
}

ObjString* vconcatenateMultipleStrings(Collector* collector, ObjString* first, va_list rest) {
    ObjString* result = first; 
    ObjString* second = NULL;
    for (;;) {
        second = va_arg(rest, ObjString*);
        if (second == NULL)
            break;
        result = concatenateStrings(collector, result, second);
    }
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
        popSafe(collector);
        result = concatenateStrings(collector, sa, sb);
    }
    return result;
}

static ObjString* strOrSelf(Collector* collector, ObjString* accumulated, Obj* container, Value value) {
    pushSafe(collector, to_vobj(accumulated));
    ObjString* result;
    if (is_obj(value) && as_obj(value) == container)
        result = concatenateCharArrays(collector, "<self>", NULL);
    else
        result = valueToString(collector, value);
    popSafe(collector);
    return result;
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
                ObjString* result = copyString(collector, "[", 1);
                for (int i = 0; i < array->values->count - 1; i++) {
                    Value val = array->values->values[i];
                    result = concatenateStrings(collector, result, strOrSelf(collector, result, obj, val));
                    result = concatenateStringAndCharArray(collector, result, ",");
                }
                if (array->values->count > 0) {
                    int index = array->values->count - 1;
                    Value val = array->values->values[index];
                    result = concatenateStrings(collector, result, strOrSelf(collector, result, obj, val));
                }
                result = concatenateStringAndCharArray(collector, result, "]");
                return result;
            }
        case OBJ_DICT:
            {
                ObjDict* dict = (ObjDict*) obj;
                ObjString* result = copyString(collector, "{", 1);
                HashMap* map = dict->map;
                for (int i = 0; i < map->capacity; i++) {
                    Entry* entry = map->entries[i];
                    while (entry != NULL) {
                        result = concatenateStrings(collector, result, 
                                strOrSelf(collector, result, obj, entry->key));
                        result = concatenateStringAndCharArray(collector, result, " => ");
                        result = concatenateStrings(collector, result, 
                                strOrSelf(collector, result, obj, entry->value));
                        result = concatenateStringAndCharArray(collector, result, ",");
                        entry = entry->next;
                    }
                }
                result = concatenateStringAndCharArray(collector, result, "}");
                return result;
            }
    }
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

void printValue(Collector* collector, Value val) {
    printf("%s", valueToCharArray(collector, val)); 
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