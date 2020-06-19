#ifndef natives_h
#define natives_h

#include "natives_common.h"

Value nativeToStr(VM* vm, int argCount, Value* args);

#define natives_h_declare \
    vmDeclareNative(vm, "tostr", &nativeToStr);

#endif
