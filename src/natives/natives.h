#ifndef natives_h
#define natives_h

#include "natives_common.h"

Value nativeToStr(VM* vm, Value* args);
Value nativeTypeOf(VM* vm, Value* args);

#define natives_h_declare \
    vmDeclareNative(vm, 1, "tostr", &nativeToStr);

#endif
