#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"
#include "services/asm_printer.h"
#include "backend/vm.h"

int main(int argc, char **argv) {
    Chunk c;
    VM vm;
    initVM(&vm);
    initChunk(&c);
    int constant = writeConstant(&c, 1.2);
    writeChunk(&c, OP_CONST, 2);
    writeChunk(&c, constant, 2);
    constant = writeConstant(&c, 5.2);
    writeChunk(&c, OP_CONST, 3);
    writeChunk(&c, constant, 3);
    writeChunk(&c, OP_ADD, 5);
    writeChunk(&c, OP_RET, 5);
    vmExecute(&vm, &c);

    printChunk(&c, "Chunk");
    return 0;
}
