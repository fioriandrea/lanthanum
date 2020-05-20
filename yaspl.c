#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"
#include "services/asm_printer.h"

int main(int argc, char **argv) {
    Chunk c;
    initChunk(&c);
    writeChunk(&c, OP_RETURN, 5);
    int constant = writeConstant(&c, 1.2);
    writeChunk(&c, OP_CONST, 2);
    writeChunk(&c, constant, 2);
    constant = writeConstant(&c, 5.2);
    writeChunk(&c, OP_CONST, 3);
    writeChunk(&c, constant, 3);
    
    printChunk(&c, "Chunk");
    return 0;
}
