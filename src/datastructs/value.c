#include <stdio.h>

#include "value.h"
#include "../util.h"
#include "../memory.h"

int isObjType(struct sValue value, ObjType type) {
    return is_obj(value) && as_obj(value)->type == type;
}

void initValueArray(struct sValueArray* valarray) {
    valarray->count = 0;
    valarray->capacity = 0;
    valarray->values = NULL;
}

int writeValueArray(Collector* collector, struct sValueArray* valarray, struct sValue value) {
    if (valarray->count + 1 >= valarray->capacity) {
        int newcap = compute_capacity(valarray->capacity);
        valarray->values = grow_array(collector, struct sValue, valarray->values, valarray->capacity, newcap);
        valarray->capacity = newcap; 
    }     
    valarray->values[valarray->count++] = value;
    return valarray->count - 1;
}

void freeValueArray(Collector* collector, struct sValueArray* valarray) {
    free_array(collector, uint8_t, valarray->values, valarray->capacity);
    initValueArray(valarray);
}

uint32_t hashValue(struct sValue value) {
    switch (value.type) {
        case VALUE_NIHL: return hash_nihl;
        case VALUE_BOOL: return hash_bool(value);
        case VALUE_NUMBER: return hash_number(value);
        case VALUE_OBJ: return as_obj(value)->hash;
    }
}

int isTruthy(struct sValue val) {
    return !(is_nihl(val) || (is_bool(val) && !as_cbool(val)) ||
            (is_number(val) && as_cnumber(val) == 0));
}

int valueInteger(struct sValue value) {
    if (!is_number(value))
        return 0;
    return is_integer(as_cnumber(value));
}

int valuesIntegers(struct sValue a, struct sValue b) {
    return valueInteger(a) && valueInteger(b);
}

int valuesEqual(struct sValue a, struct sValue b) {
    if (a.type != b.type)
        return 0;
    switch (a.type) {
        case VALUE_NIHL: return 1;
        case VALUE_NUMBER: return as_cnumber(a) == as_cnumber(b);
        case VALUE_BOOL: return as_cbool(a) == as_cbool(b);
        case VALUE_OBJ: return as_obj(a) == as_obj(b);
    }
}

int valuesConcatenable(struct sValue a, struct sValue b) {
    return is_string(a) && is_string(b);
}

int valuesNumbers(struct sValue a, struct sValue b) {
    return is_number(a) && is_number(b);
}

struct sValue concatenate(Collector* collector, struct sValue a, struct sValue b) {
    if (!is_obj(a) || !is_obj(b)) {
        return to_vobj(newError(collector, "cannot concatenate non objects", NULL));
    }
    return to_vobj(concatenateObjects(collector, as_obj(a), as_obj(b)));
}

void printValue(Collector* collector, struct sValue val) {
    printf("%s", valueToCharArray(collector, val)); 
}

void markValue(Collector* collector, struct sValue value) {
    if (!is_obj(value))
        return;
    markObject(collector, as_obj(value));
}

void markValueArray(Collector* collector, struct sValueArray* values) {
    for (int i = 0; i < values->count; i++)
        markValue(collector, values->values[i]);
}

Value indexGetValue(Collector* collector, struct sValue arrayLike, struct sValue index) {
    if (!is_obj(arrayLike)) {
        return to_vobj(newError(collector, "value not indexable", NULL));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result = to_vnihl();
    indexGetObject(collector, arrayObj, &index, &result);
    return result;
}

Value indexSetValue(Collector* collector, struct sValue arrayLike, struct sValue index, struct sValue value) {
    if (!is_obj(arrayLike)) {
        return to_vobj(newError(collector, "value not indexable", NULL));
    }
    Obj* arrayObj = as_obj(arrayLike);
    Value result;
    indexSetObject(collector, arrayObj, &index, &value, &result);
    return result;
}

ObjString* valueToString(Collector* collector, struct sValue value) {
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

char* valueToCharArray(Collector* collector, struct sValue value) {
    return valueToString(collector, value)->chars;
}
