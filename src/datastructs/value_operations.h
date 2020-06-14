#ifndef value_operations
#define value_operations

#include <stdarg.h>

#include "value.h"

void indexGetObject(Collector* collector, Obj* array, Value* index, Value* result);
void indexSetObject(Collector* collector, Obj* array, Value* index, Value* value, Value* result);
ObjString* concatenateStringAndCharArray(Collector* collector, ObjString* str, char* carr);
ObjString* concatenateStringAndCharArraySafe(Collector* collector, ObjString* str, char* carr);
ObjString* objectToString(Collector* collector, Obj* obj);
ObjString* concatenateMultipleCharArrays(Collector* collector, char* first, ...);
ObjString* vconcatenateMultipleCharArrays(Collector* collector, char* first, va_list rest);
ObjError* makeErrorFromStrings(Collector* collector, char* first, ...);
Obj* concatenateObjects(Collector* collector, Obj* a, Obj* b);
void arrayPush(Collector* collector, ObjArray* array, Value* value);
int indexSetDict(Collector* collector, ObjDict* dict, Value* key, Value* value);
int indexGetDict(ObjDict* dict, Value* key, Value* result);
int indexSetArray(Collector* collector, ObjArray* array, Value* index, Value* value, Value* result);
int indexGetArray(Collector* collector, ObjArray* array, Value* index, Value* result);
int indexGetString(Collector* collector, ObjString* string, Value* index, Value* result);


Value indexGetValue(Collector* collector, Value arrayLike, Value index);
Value indexSetValue(Collector* collector, Value arrayLike, Value index, Value value);
ObjString* valueToString(Collector* collector, Value value);
void printValue(Collector* collector, Value val);
int isTruthy(Value val); 
int valueInteger(Value value);
int valuesIntegers(Value a, Value b); 
int valuesEqual(Value a, Value b); 
int valuesConcatenable(Value a, Value b); 
int valuesNumbers(Value a, Value b); 
Value concatenate(Collector* collector, Value a, Value b); 

#endif
