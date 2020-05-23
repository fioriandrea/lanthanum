#ifndef value_h
#define value_h

#include "object.h"

typedef enum {
    VALUE_NIHL,
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int boolean;
        double number;
        Obj* obj;
    } as; 
} Value;

#define is_nihl(value) ((value).type == VALUE_NIHL)
#define is_bool(value) ((value).type == VALUE_BOOL)
#define is_number(value) ((value).type == VALUE_NUMBER)
#define is_obj(value) ((value).type == VALUE_OBJ)

#define as_cbool(value) ((value).as.boolean)
#define as_cnumber(value) ((value).as.number)
#define as_obj(value) ((value).as.obj)

#define to_vbool(cbool) ((Value) {VALUE_BOOL, {.boolean = (cbool)}})
#define to_vnihl() ((Value) {VALUE_NIHL, {.number = 0}}) 
#define to_vnumber(cnumber) ((Value) {VALUE_NUMBER, {.number = (cnumber)}})
#define to_vobj(object) ((Value) {VALUE_OBJ, {.obj = ((Obj*) object)}})

#define is_string(value) isObjType(value, OBJ_STRING)

#define as_string(value) ((ObjString*) as_obj(value))
#define as_cstring(value) (as_string(value)->chars)

int isObjType(Value value, ObjType type);

typedef struct {
    int count;
    int capacity;
    Value* values;
} ValueArray;

#define hash_bool(b) hash_int(as_cbool(b) + 31)
#define hash_nihl hash_int(42)
#define hash_number(n) hash_double(as_cnumber(n))

uint32_t hashValue(Value val);

static inline uint32_t get_value_hash(Value val) {
    return is_obj(val) ? as_obj(val)->hash : hashValue(val);
}

void initValueArray(ValueArray* valarray);
int writeValueArray(ValueArray* valarray, Value value);
void freeValueArray(ValueArray* valarray);
int isTruthy(Value val); 
int valuesIntegers(Value a, Value b); 
int valuesEqual(Value a, Value b); 
int valuesConcatenable(Value a, Value b); 
int valuesNumbers(Value a, Value b); 
Value concatenate(Value a, Value b); 
void printValue(Value val);

#endif
