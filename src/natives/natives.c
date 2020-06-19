#include "natives.h"

Value nativeToStr(VM* vm, int argCount, Value* args) {
    if (argCount != 1) {
        ObjString* message = copyNoLengthString(vm->collector, "expected 1 argument, but got ");
        pushSafeObj(vm->collector, message);
        message = concatenateStringsSafe(vm->collector, 
                message, valueToString(vm->collector, to_vnumber(argCount)));
        popSafe(vm->collector);

        pushSafeObj(vm->collector, message);
        ObjError* error = newError(vm->collector, message);
        popSafe(vm->collector);
        return to_vobj(error);
    }
    return to_vobj(valueToString(vm->collector, args[0]));
}
