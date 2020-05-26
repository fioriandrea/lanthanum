#include <string.h>
#include <stdio.h>

#include "object.h"
#include "../util.h"
#include "../memory.h"

#define allocate_obj(collector, type, typeenum) \
    ((type*) allocateObj(collector, typeenum, sizeof(type)))

Obj* allocateObj(Collector* collector, ObjType type, size_t size) {
    Obj* obj = allocate_pointer(collector, Obj, size);
    obj->type = type;
    obj->next = collector->objects;
    obj->hash = hash_pointer(obj);
    collector->objects = obj;
    return obj;
}

ObjString* copyString(Collector* collector, char* chars, int length) {
    ObjString* str;
    if ((str = containsStringDeepEqual(&collector->interned, chars, length)) != NULL) {
        return str;
    }
    char* copied = allocate_block(collector, char, length + 1);
    memcpy(copied, chars, length);
    copied[length] = '\0';
    return takeString(collector, copied, length);
}

ObjString* takeString(Collector* collector, char* chars, int length) {
    ObjString* str;
    if ((str = containsStringDeepEqual(&collector->interned, chars, length)) != NULL) {
        free_block(collector, char, chars, length);
        return str;
    }
    ObjString* string = allocate_obj(collector, ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    ((Obj*) string)->hash = hash_string(chars, length);
    mapPut(collector, &collector->interned, to_vobj(string), to_vnihl());
    return string;
}

void freeObject(Collector* collector, Obj* object) {
    switch (object->type) {                                 
        case OBJ_STRING: 
            {                                    
                ObjString* string = (ObjString*) object;             
                free_array(collector, char, string->chars, string->length + 1);
                free_pointer(collector, object, sizeof(ObjString));                            
                break;                                              
            }                                                     
    }
}

void printObj(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            printf("%s", ((ObjString*) obj)->chars);
            break;
    }
}
