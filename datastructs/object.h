#ifndef object_h
#define object_h

#include "../commontypes.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

struct sObj {
    ObjType type;
    uint32_t hash;
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
    Chunk* chunk;
    int upvalueCount;
} ObjFunction;

typedef struct {
    Obj obj;
    Value* value;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
} ObjClosure;

ObjString* copyString(Collector* collector, char* chars, int length);
ObjString* takeString(Collector* collector, char* chars, int length);
ObjFunction* newFunction(Collector* collector);
ObjClosure* newClosure(Collector* collector, ObjFunction* function);
ObjUpvalue* newUpvalue(Collector* collector, Value* value);
void freeObject(Collector* collector, Obj* object);
void printObj(Obj* obj);

#endif
