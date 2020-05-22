#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"
#include "services/asm_printer.h"
#include "services/token_printer.h"
#include "services/memory.h"
#include "vm.h"
#include "lexer.h"

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {                                      
        fprintf(stderr, "cannot open file at path \"%s\"\n", path);
        exit(1);                                              
    }   

    fseek(file, 0L, SEEK_END);                                     
    size_t fileSize = ftell(file);                                 
    rewind(file);                                                  

    char* buffer = (char*) malloc(fileSize + 2); // + 2 because we have to append \n and \0                     
    if (buffer == NULL) {                                          
        fprintf(stderr, "have not enough memory to read file at path \"%s\"\n", path);
        exit(1);                                                    
    } 
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {                                    
        fprintf(stderr, "cannot read file at path \"%s\"\n", path);      
        exit(1);       
    }
    buffer[bytesRead] = '\n';
    buffer[bytesRead + 1] = '\0';                                      

    fclose(file);                                                  
    return buffer;
}

static void runFile(const char* fname, VM* vm) {
    char* source = readFile(fname);
    ExecutionResult result = vmExecute(vm, source);
    if (result == EXEC_COMPILE_ERROR || result == EXEC_RUNTIME_ERROR) 
        exit(1); 
}

int main(int argc, char **argv) {
    VM vm;
    initVM(&vm);
    if (argc <= 1) {
        fprintf(stderr, "error: missing files names\n");
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        runFile(argv[i], &vm);
    }
    freeVM(&vm);
    return 0;
}
