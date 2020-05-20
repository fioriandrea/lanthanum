#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"

int main(int argc, char **argv) {
    Chunk c;
    initChunk(&c);
    writeChunk(&c, OP_RETURN);
    return 0;
}
