#include "natives.h"

Value nativeToStr(VM* vm, Value* args) {
    return to_vobj(valueToString(vm->collector, args[0]));
}
