#ifndef value_h
#define value_h

#include "object.h"
#include "../commontypes.h"

typedef enum {
    VALUE_NIHL,
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_OBJ,
} ValueType;

struct sValue {
    ValueType type;
    union {
        int boolean;
        double number;
        Obj* obj;
    } as; 
};

#define is_nihl(value) ((value).type == VALUE_NIHL)
#define is_bool(value) ((value).type == VALUE_BOOL)
#define is_number(value) ((value).type == VALUE_NUMBER)
#define is_obj(value) ((value).type == VALUE_OBJ)

#define as_cbool(value) ((value).as.boolean)
#define as_cnumber(value) ((value).as.number)
#define as_obj(value) ((value).as.obj)

#define to_vbool(cbool) ((struct sValue) {VALUE_BOOL, {.boolean = (cbool)}})
#define to_vnihl() ((struct sValue) {VALUE_NIHL, {.number = 0}}) 
#define to_vnumber(cnumber) ((struct sValue) {VALUE_NUMBER, {.number = (cnumber)}})
#define to_vobj(object) ((struct sValue) {VALUE_OBJ, {.obj = ((Obj*) object)}})

#define is_string(value) isObjType(value, OBJ_STRING)
#define is_function(value) isObjType(value, OBJ_FUNCTION)
#define is_closure(value) isObjType(value, OBJ_CLOSURE)
#define is_upvalue(value) isObjType(value, OBJ_UPVALUE)
#define is_array(value) isObjType(value, OBJ_ARRAY)
#define is_dict(value) isObjType(value, OBJ_DICT)
#define is_error(value) isObjType(value, OBJ_ERROR)

#define as_function(value) ((ObjFunction*) as_obj(value))
#define as_closure(value) ((ObjClosure*) as_obj(value))
#define as_upvalue(value) ((ObjUpvalue*) as_obj(value))
#define as_error(value) ((ObjError*) as_obj(value))
#define as_string(value) ((ObjString*) as_obj(value))
#define as_array(value) ((ObjArray*) as_obj(value))
#define as_dict(value) ((ObjDict*) as_obj(value))
#define as_cstring(value) (as_string(value)->chars)

int isObjType(struct sValue value, ObjType type);

struct sValueArray {
    int count;
    int capacity;
    struct sValue* values;
};

#define hash_bool(b) hash_int(as_cbool(b) + 31)
#define hash_nihl hash_int(42)
#define hash_number(n) hash_double(as_cnumber(n))

uint32_t hashValue(struct sValue val);

static inline uint32_t get_value_hash(struct sValue val) {
    return is_obj(val) ? as_obj(val)->hash : hashValue(val);
}

void initValueArray(ValueArray* valarray);
int writeValueArray(Collector* collector, struct sValueArray* valarray, struct sValue value);
void freeValueArray(Collector* collector, struct sValueArray* valarray);
int isTruthy(struct sValue val); 
int valueInteger(struct sValue value);
int valuesIntegers(struct sValue a, struct sValue b); 
int valuesEqual(struct sValue a, struct sValue b); 
int valuesConcatenable(struct sValue a, struct sValue b); 
int valuesNumbers(struct sValue a, struct sValue b); 
struct sValue concatenate(Collector* collector, struct sValue a, struct sValue b); 
void printValue(struct sValue val);
void markValueArray(Collector* collector, struct sValueArray* values);
void markValue(Collector* collector, struct sValue value);
Value indexGetValue(Collector* collector, struct sValue arrayLike, struct sValue index);
Value indexSetValue(Collector* collector, struct sValue arrayLike, struct sValue index, struct sValue value);

#endif
