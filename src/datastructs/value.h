#ifndef value_h
#define value_h

#include "../commontypes.h"
#include <stdarg.h>

typedef struct sValue Value;
typedef struct sValueArray ValueArray;

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_ARRAY,
    OBJ_DICT,
    OBJ_ERROR,
} ObjType;

struct sObj {
    ObjType type;
    uint32_t hash;
    int marked;
    struct sObj* next;
};

typedef struct sObj Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

typedef struct {
    Obj obj;
    int arity;
    ObjString* name;
    Bytecode* bytecode;
    int upvalueCount;
} ObjFunction;

struct sObjUpvalue {
    Obj obj;
    Value* value;
    Value* closed;
    struct sObjUpvalue* next;
};

typedef struct sObjUpvalue ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString* message;
    Value* payload;
} ObjError;

typedef struct {
    Obj obj;
    ValueArray* values;
} ObjArray;

typedef struct {
    Obj obj;
    HashMap* map;
} ObjDict;

ObjString* copyString(Collector* collector, char* chars, int length);
ObjString* copyNoLengthString(Collector* collector, char* chars);
ObjString* takeString(Collector* collector, char* chars, int length);
ObjFunction* newFunction(Collector* collector);
ObjClosure* newClosure(Collector* collector, ObjFunction* function);
ObjUpvalue* newUpvalue(Collector* collector, Value* value);
ObjArray* newArray(Collector* collector);
ObjDict* newDict(Collector* collector);
ObjError* newError(Collector* collector, char* first, ...);
void closeUpvalue(ObjUpvalue* upvalue);
void freeObject(Collector* collector, Obj* object);
void markObject(Collector* collector, Obj* obj);
void blackenObject(Collector* collector, Obj* obj);

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

#define to_vbool(cbool) ((Value) {VALUE_BOOL, {.boolean = (cbool)}})
#define to_vnihl() ((Value) {VALUE_NIHL, {.number = 0}}) 
#define to_vnumber(cnumber) ((Value) {VALUE_NUMBER, {.number = (cnumber)}})
#define to_vobj(object) ((Value) {VALUE_OBJ, {.obj = ((Obj*) object)}})

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

int isObjType(Value value, ObjType type);

struct sValueArray {
    int count;
    int capacity;
    Value* values;
};

#define hash_bool(b) hash_int(as_cbool(b) + 31)
#define hash_nihl hash_int(42)
#define hash_number(n) hash_double(as_cnumber(n))

uint32_t hashValue(Value val);

static inline uint32_t get_value_hash(Value val) {
    return is_obj(val) ? as_obj(val)->hash : hashValue(val);
}

void initValueArray(ValueArray* valarray);
int writeValueArray(Collector* collector, ValueArray* valarray, Value value);
void freeValueArray(Collector* collector, ValueArray* valarray);
void markValueArray(Collector* collector, ValueArray* values);
void markValue(Collector* collector, Value value);

#endif
