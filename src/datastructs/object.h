#ifndef object_h
#define object_h

#include "../commontypes.h"

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
ObjString* takeString(Collector* collector, char* chars, int length);
ObjFunction* newFunction(Collector* collector);
ObjClosure* newClosure(Collector* collector, ObjFunction* function);
ObjUpvalue* newUpvalue(Collector* collector, Value* value);
ObjArray* newArray(Collector* collector);
ObjDict* newDict(Collector* collector);
ObjError* newError(Collector* collector, char* first, ...);
void closeUpvalue(ObjUpvalue* upvalue);
void freeObject(Collector* collector, Obj* object);
void printObj(Obj* obj);
void markObject(Collector* collector, Obj* obj);
void blackenObject(Collector* collector, Obj* obj);
void indexGetObject(Collector* collector, Obj* array, Value* index, Value* result);
void indexSetObject(Collector* collector, Obj* array, Value* index, Value* value, Value* result);
Obj* concatenateObjects(Collector* collector, Obj* a, Obj* b);
void arrayPush(Collector* collector, ObjArray* array, Value* value);
int dictPut(Collector* collector, ObjDict* dict, Value* key, Value* value);
int dictGet(ObjDict* dict, Value* key, Value* result);

#endif