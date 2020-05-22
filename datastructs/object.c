#include <string.h>

#include "object.h"
#include "../services/memory.h"

#define allocate_obj(type, typeenum) \
    ((type*) allocateObj(typeenum, sizeof(type)))

Obj* allocateObj(ObjType type, size_t size) {
    Obj* obj = allocate_pointer(Obj, size);
    obj->type = type;
    return obj;
}

ObjString* copyString(char* chars, int length) {
    char* copied = allocate_block(char, length + 1);
    memcpy(copied, chars, length);
    copied[length] = '\0';
    return takeString(copied, length);
}

ObjString* takeString(char* chars, int length) {
    ObjString* string = allocate_obj(ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    return string;
}
