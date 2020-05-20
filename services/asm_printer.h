#ifndef asm_printer_h
#define asm_printer_h

#include "../datastructs/chunk.h"

void printChunk(Chunk* chunk, char* name);
int printInstruction(OpCode code, int offset);

#endif
