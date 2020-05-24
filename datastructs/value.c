#include <stdio.h>

#include "value.h"
#include "../util.h"
#include "../memory.h"

#define debug_hash_codes

int isObjType(Value value, ObjType type) {
    return is_obj(value) && as_obj(value)->type == type;
}

void initValueArray(ValueArray* valarray) {
    valarray->count = 0;
    valarray->capacity = 0;
    valarray->values = NULL;
}

int writeValueArray(ValueArray* valarray, Value value) {
    if (valarray->count + 1 >= valarray->capacity) {
        int newcap = compute_capacity(valarray->capacity);
        valarray->values = grow_array(Value, valarray->values, valarray->capacity, newcap);
        valarray->capacity = newcap; 
    }     
    valarray->values[valarray->count++] = value;
    return valarray->count - 1;
}

void freeValueArray(ValueArray* valarray) {
    free_array(uint8_t, valarray->values, valarray->capacity);
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

int valuesIntegers(Value a, Value b) {
    return is_integer(as_cnumber(a)) && is_integer(as_cnumber(b));
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

Value concatenate(Value a, Value b) {
    ObjString* sa = as_string(a);
    ObjString* sb = as_string(b);
    int length = sa->length + sb->length;
    char* chars = allocate_block(char, length);
    memcpy(chars, sa->chars, sa->length);
    memcpy(chars + sa->length, sb->chars, sb->length);
    chars[length] = '\0'; 
    return to_vobj(takeString(chars, length));
}

void printValue(Value val) {
#ifdef debug_hash_codes
    printf("hash[%zu]:", hashValue(val));
#endif
    switch (val.type) {
        case VALUE_BOOL:
            printf("%s", as_cbool(val) ? "true" : "false");
            break;
        case VALUE_NUMBER:
            printf("%g", as_cnumber(val));
            break;
        case VALUE_NIHL:
            printf("nihl");
            break;
        case VALUE_OBJ:
            printObj(as_obj(val));
            break;
    }
}
