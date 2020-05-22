#include <string.h>

#include "./datastructs/value.h"
#include "./services/memory.h"

int isInteger(double n) {
    return (int) n == n;
}

int isTruthy(Value val) {
    return !(is_nihl(val) || (is_bool(val) && !as_cbool(val)) ||
            (is_number(val) && as_cnumber(val) == 0));
}

int valuesIntegers(Value a, Value b) {
    return isInteger(as_cnumber(a)) && isInteger(as_cnumber(b));
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

