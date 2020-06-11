#ifndef asm_printer_h
#define asm_printer_h

#include "../datastructs/bytecode.h"

void printBytecode(Bytecode* bytecode, char* name);
int printInstruction(Bytecode* bytecode, OpCode code, int offset);

#endif
