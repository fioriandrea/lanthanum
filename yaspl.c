#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./memory.h"
#include "vm.h"
#include "./compilation_pipeline/compiler.h"

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

static void runFile(const char* fname, VM* vm, Compiler* compiler, Collector* collector) {
    char* source = readFile(fname);
    ObjFunction* function = compile(compiler, collector, source);
    if (function == NULL) { // compile error
        exit(1);
    }
    int runtimeResult = vmExecute(vm, collector, function);
    if (!runtimeResult) { // runtime error
        exit(1); 
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "error: missing files names\n");
        exit(1);
    }
    Collector collector;
    initCollector(&collector);
    VM vm;
    Compiler compiler;

    runFile(argv[1], &vm, &compiler, &collector);
    return 0;
}
