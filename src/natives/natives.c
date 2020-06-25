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
    if (!is_string(arg))
        return to_vobj(newErrorFromCharArray(vm->collector, "passed non string to system"));
    return to_vnumber(system(as_cstring(arg)));
}

Value nativeLen(VM* vm, Value* args) {
    Value arg = args[0];
    if (!is_string(arg) && !is_array(arg))
        return to_vobj(newErrorFromCharArray(vm->collector, "length computable only for strings and arrays"));
    Obj* obj = as_obj(arg);
    return to_vnumber(arrayLikeLength(obj));
}

Value nativePairList(VM* vm, Value* args) {
    Value arg = args[0];
    if (!valueIndexable(arg))
        return to_vobj(newErrorFromCharArray(vm->collector, "value not indexable"));
    Obj* obj = as_obj(arg);
    return to_vobj(pairList(vm->collector, obj));
}
