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

void initValueArray(ValueArray* valarray);
int writeValueArray(ValueArray* valarray, Value value);
void freeValueArray(ValueArray* valarray);

#endif
