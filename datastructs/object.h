#ifndef object_h
#define object_h

typedef enum {
    OBJ_STRING,
} ObjType;

struct sObj {
    ObjType type;
    struct sObj* next;
};

typedef struct sObj Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

ObjString* copyString(char* chars, int length);
ObjString* takeString(char* chars, int length);
void freeObject(Obj* object);

#endif
