#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"
#include "services/asm_printer.h"

int main(int argc, char **argv) {
    Chunk c;
    initChunk(&c);
    writeChunk(&c, OP_RETURN);
    int constant = writeConstant(&c, 1.2);
    writeChunk(&c, OP_CONST);
    writeChunk(&c, constant);
    
    printChunk(&c, "Chunk");
    return 0;
}
