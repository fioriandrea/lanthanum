#ifndef value_operations_h
#define value_operations_h

#include "./datastructs/value.h"

int isInteger(double n); 
int isTruthy(Value val); 
int valuesIntegers(Value a, Value b); 
int valuesEqual(Value a, Value b); 
int valuesConcatenable(Value a, Value b); 
int valuesNumbers(Value a, Value b); 
Value concatenate(Value a, Value b); 

#endif
