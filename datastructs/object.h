#ifndef object_h
#define object_h

#include "../commontypes.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
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
} ObjFunction;

ObjString* copyString(Collector* collector, char* chars, int length);
ObjString* takeString(Collector* collector, char* chars, int length);
ObjFunction* newFunction(Collector* collector);
void freeObject(Collector* collector, Obj* object);
void printObj(Obj* obj);

#endif
