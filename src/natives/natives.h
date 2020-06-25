#ifndef natives_h
#define natives_h

#include "natives_common.h"

Value nativeToStr(VM* vm, Value* args);
Value nativeTypeOf(VM* vm, Value* args);
Value nativeTypeOfObject(VM* vm, Value* args);
Value nativeSystem(VM* vm, Value* args);
Value nativeLen(VM* vm, Value* args);
Value nativePairList(VM* vm, Value* args);

#define natives_h_declare(vm) \
    vmDeclareNative(vm, 1, "tostr", &nativeToStr); \
    vmDeclareNative(vm, 1, "typeof", &nativeTypeOf); \
    vmDeclareNative(vm, 1, "typeofobj", &nativeTypeOfObject); \
    vmDeclareNative(vm, 1, "system", &nativeSystem); \
    vmDeclareNative(vm, 1, "len", &nativeLen); \
    vmDeclareNative(vm, 1, "pairList", &nativePairList); \

#endif
