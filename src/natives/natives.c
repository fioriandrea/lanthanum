#include <stdlib.h>

#include "natives.h"

Value nativeToStr(VM* vm, Value* args) {
    return to_vobj(valueToString(vm->collector, args[0]));
}

Value nativeTypeOf(VM* vm, Value* args) {
    Value arg = args[0];
    switch (arg.type) {
        case VALUE_NUMBER:
            return to_vobj(copyNoLengthString(vm->collector, "number"));
        case VALUE_BOOL:
            return to_vobj(copyNoLengthString(vm->collector, "boolean"));
        case VALUE_NIHL:
            return to_vobj(copyNoLengthString(vm->collector, "nihl"));
        case VALUE_OBJ:
            return to_vobj(copyNoLengthString(vm->collector, "object")); 
    }
}

Value nativeTypeOfObject(VM* vm, Value* args) {
    Obj* arg = as_obj(args[0]);
    switch (arg->type) {
        case OBJ_STRING: 
            return to_vobj(copyNoLengthString(vm->collector, "string"));
        case OBJ_FUNCTION:
            return to_vobj(copyNoLengthString(vm->collector, "function"));
        case OBJ_NATIVE_FUNCTION:
            return to_vobj(copyNoLengthString(vm->collector, "native"));
        case OBJ_CLOSURE:
            return to_vobj(copyNoLengthString(vm->collector, "closure"));
        case OBJ_UPVALUE:
            return to_vobj(copyNoLengthString(vm->collector, "upvalue"));
        case OBJ_ERROR:
            return to_vobj(copyNoLengthString(vm->collector, "error"));
        case OBJ_ARRAY:
            return to_vobj(copyNoLengthString(vm->collector, "array"));
        case OBJ_DICT:
            return to_vobj(copyNoLengthString(vm->collector, "dictionary"));
        default:
            return to_vobj(newErrorFromCharArray(vm->collector, "value is not an object"));
    }
}

Value nativeSystem(VM* vm, Value* args) {
    Value arg = args[0];
    if (!is_obj(arg) || !is_string(arg))
        return to_vobj(newErrorFromCharArray(vm->collector, "passed non string to system"));
    return to_vnumber(system(as_cstring(arg)));
}
