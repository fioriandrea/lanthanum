#ifndef value_dump
#define value_dump

#include <stdio.h>

#include "../datastructs/value.h"

static void dumpObj(Obj* obj);

static void dumpValue(Value val) {
    switch (val.type) {
        case VALUE_BOOL:
            printf("%s", as_cbool(val) ? "true" : "false");
            break;
        case VALUE_NUMBER:
            printf("%g", as_cnumber(val));
            break;
        case VALUE_NIHL:
            printf("nihl");
            break;
        case VALUE_OBJ:
            dumpObj(as_obj(val));
            break;
    }
}

static void dumpObj(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:
            printf("%s", ((ObjString*) obj)->chars);
            break;
        case OBJ_FUNCTION:
            {
                ObjFunction* function = (ObjFunction*) obj;
                if (function->name == NULL)
                    printf("<main script>");
                else
                    printf("<%s function>", function->name->chars);
                break;
            }
        case OBJ_CLOSURE:
            {
                ObjClosure* closure = (ObjClosure*) obj;
                printf("closure ");
                dumpObj((Obj*) closure->function);
                break;
            }
        case OBJ_UPVALUE:
            {
                ObjUpvalue* upvalue = (ObjUpvalue*) obj;
                printf("upvalue ");
                dumpValue(*upvalue->value);
                break;
            }
        case OBJ_ERROR:
            {
                ObjError* error = (ObjError*) obj;
                dumpObj((Obj*) error->message);
                if (error->payload != NULL) {
                    printf(" {");
                    dumpValue(*error->payload);
                    printf("}");
                }
                break;
            }
        case OBJ_ARRAY:
            {
                printf("[array %p]", (void*) obj);
                break;
            }
        case OBJ_DICT:
            {
                printf("[dict %p]", (void*) obj);
                break;
            }
    }
}


#endif
