#ifndef object_h
#define object_h

typedef enum {
    OBJ_STRING,
} ObjType;

typedef struct {
    ObjType type;
} Obj;

typedef struct {
    Obj obj;
    int length;
    char* chars;
} ObjString;

ObjString* copyString(char* chars, int length);
ObjString* takeString(char* chars, int length);

#endif
