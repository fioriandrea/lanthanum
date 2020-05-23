#ifndef object_h
#define object_h

#include "../standardtypes.h"

typedef enum {
    OBJ_STRING,
} ObjType;

struct sObj {
    ObjType type;
    uint32_t hash;
    struct sObj* next;
};

typedef struct sObj Obj;

extern Obj* objList;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

ObjString* copyString(char* chars, int length);
ObjString* takeString(char* chars, int length);
void freeObject(Obj* object);
void freeObjList();
void printObj(Obj* obj);

#endif
