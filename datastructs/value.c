#include "value.h"
#include "../services/memory.h"

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
