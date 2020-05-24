#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datastructs/chunk.h"
#include "./debug/asm_printer.h"
#include "./debug/token_printer.h"
#include "memory.h"
#include "vm.h"
#include "./compilation_pipeline/lexer.h"
#include "./debug/map_printer.h"
#include "./datastructs/hash_map.h"

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

int main(int argc, char **argv) {/*
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
    freeObjList();*/
    HashMap m;
    HashMap* map = &m;
    initMap(map);

mapPut(map, to_vobj(copyString("ciao 1", 6)), to_vnumber(1));
mapPut(map, to_vobj(copyString("ciao 2", 6)), to_vnumber(2));
mapPut(map, to_vobj(copyString("ciao 3", 6)), to_vnumber(3));
mapPut(map, to_vobj(copyString("ciao 4", 6)), to_vnumber(4));
mapPut(map, to_vobj(copyString("ciao 5", 6)), to_vnumber(5));
mapPut(map, to_vobj(copyString("ciao 6", 6)), to_vnumber(6));
mapPut(map, to_vobj(copyString("ciao 7", 6)), to_vnumber(7));
mapPut(map, to_vobj(copyString("ciao 8", 6)), to_vnumber(8));
mapPut(map, to_vobj(copyString("ciao 9", 6)), to_vnumber(9));
mapPut(map, to_vobj(copyString("ciao 10", 7)), to_vnumber(10));
mapPut(map, to_vobj(copyString("ciao 11", 7)), to_vnumber(11));
mapPut(map, to_vobj(copyString("ciao 12", 7)), to_vnumber(12));
mapPut(map, to_vobj(copyString("ciao 13", 7)), to_vnumber(13));
mapPut(map, to_vobj(copyString("ciao 14", 7)), to_vnumber(14));
mapPut(map, to_vobj(copyString("ciao 15", 7)), to_vnumber(15));
mapPut(map, to_vobj(copyString("ciao 16", 7)), to_vnumber(16));
mapPut(map, to_vobj(copyString("ciao 17", 7)), to_vnumber(17));
mapPut(map, to_vobj(copyString("ciao 18", 7)), to_vnumber(18));
mapPut(map, to_vobj(copyString("ciao 19", 7)), to_vnumber(19));
mapPut(map, to_vobj(copyString("ciao 20", 7)), to_vnumber(20));
mapPut(map, to_vobj(copyString("ciao 21", 7)), to_vnumber(21));
mapPut(map, to_vobj(copyString("ciao 22", 7)), to_vnumber(22));
mapPut(map, to_vobj(copyString("ciao 23", 7)), to_vnumber(23));
mapPut(map, to_vobj(copyString("ciao 24", 7)), to_vnumber(24));
mapPut(map, to_vobj(copyString("ciao 25", 7)), to_vnumber(25));
mapPut(map, to_vobj(copyString("ciao 26", 7)), to_vnumber(26));
mapPut(map, to_vobj(copyString("ciao 27", 7)), to_vnumber(27));
mapPut(map, to_vobj(copyString("ciao 28", 7)), to_vnumber(28));
mapPut(map, to_vobj(copyString("ciao 29", 7)), to_vnumber(29));
mapPut(map, to_vobj(copyString("ciao 30", 7)), to_vnumber(30));
mapPut(map, to_vobj(copyString("ciao 31", 7)), to_vnumber(31));
mapPut(map, to_vobj(copyString("ciao 32", 7)), to_vnumber(32));
mapPut(map, to_vobj(copyString("ciao 33", 7)), to_vnumber(33));
mapPut(map, to_vobj(copyString("ciao 34", 7)), to_vnumber(34));
mapPut(map, to_vobj(copyString("ciao 35", 7)), to_vnumber(35));
mapPut(map, to_vobj(copyString("ciao 36", 7)), to_vnumber(36));
mapPut(map, to_vobj(copyString("ciao 37", 7)), to_vnumber(37));
mapPut(map, to_vobj(copyString("ciao 38", 7)), to_vnumber(38));
mapPut(map, to_vobj(copyString("ciao 39", 7)), to_vnumber(39));
mapPut(map, to_vobj(copyString("ciao 40", 7)), to_vnumber(40));
mapPut(map, to_vobj(copyString("ciao 41", 7)), to_vnumber(41));
mapPut(map, to_vobj(copyString("ciao 42", 7)), to_vnumber(42));
mapPut(map, to_vobj(copyString("ciao 43", 7)), to_vnumber(43));
mapPut(map, to_vobj(copyString("ciao 44", 7)), to_vnumber(44));
mapPut(map, to_vobj(copyString("ciao 45", 7)), to_vnumber(45));
mapPut(map, to_vobj(copyString("ciao 46", 7)), to_vnumber(46));
mapPut(map, to_vobj(copyString("ciao 47", 7)), to_vnumber(47));
mapPut(map, to_vobj(copyString("ciao 48", 7)), to_vnumber(48));
mapPut(map, to_vobj(copyString("ciao 49", 7)), to_vnumber(49));
mapPut(map, to_vobj(copyString("ciao 50", 7)), to_vnumber(50));
mapPut(map, to_vobj(copyString("ciao 51", 7)), to_vnumber(51));
mapPut(map, to_vobj(copyString("ciao 52", 7)), to_vnumber(52));
mapPut(map, to_vobj(copyString("ciao 53", 7)), to_vnumber(53));
mapPut(map, to_vobj(copyString("ciao 54", 7)), to_vnumber(54));
mapPut(map, to_vobj(copyString("ciao 55", 7)), to_vnumber(55));
mapPut(map, to_vobj(copyString("ciao 56", 7)), to_vnumber(56));
mapPut(map, to_vobj(copyString("ciao 57", 7)), to_vnumber(57));
mapPut(map, to_vobj(copyString("ciao 58", 7)), to_vnumber(58));
mapPut(map, to_vobj(copyString("ciao 59", 7)), to_vnumber(59));
mapPut(map, to_vobj(copyString("ciao 60", 7)), to_vnumber(60));
mapPut(map, to_vobj(copyString("ciao 61", 7)), to_vnumber(61));
mapPut(map, to_vobj(copyString("ciao 62", 7)), to_vnumber(62));
mapPut(map, to_vobj(copyString("ciao 63", 7)), to_vnumber(63));
mapPut(map, to_vobj(copyString("ciao 64", 7)), to_vnumber(64));
mapPut(map, to_vobj(copyString("ciao 65", 7)), to_vnumber(65));
mapPut(map, to_vobj(copyString("ciao 66", 7)), to_vnumber(66));
mapPut(map, to_vobj(copyString("ciao 67", 7)), to_vnumber(67));
mapPut(map, to_vobj(copyString("ciao 68", 7)), to_vnumber(68));
mapPut(map, to_vobj(copyString("ciao 69", 7)), to_vnumber(69));
mapPut(map, to_vobj(copyString("ciao 70", 7)), to_vnumber(70));
mapPut(map, to_vobj(copyString("ciao 71", 7)), to_vnumber(71));
mapPut(map, to_vobj(copyString("ciao 72", 7)), to_vnumber(72));
mapPut(map, to_vobj(copyString("ciao 73", 7)), to_vnumber(73));
mapPut(map, to_vobj(copyString("ciao 74", 7)), to_vnumber(74));
mapPut(map, to_vobj(copyString("ciao 75", 7)), to_vnumber(75));
mapPut(map, to_vobj(copyString("ciao 76", 7)), to_vnumber(76));
mapPut(map, to_vobj(copyString("ciao 77", 7)), to_vnumber(77));
mapPut(map, to_vobj(copyString("ciao 78", 7)), to_vnumber(78));
mapPut(map, to_vobj(copyString("ciao 79", 7)), to_vnumber(79));
mapPut(map, to_vobj(copyString("ciao 80", 7)), to_vnumber(80));
mapPut(map, to_vobj(copyString("ciao 81", 7)), to_vnumber(81));
mapPut(map, to_vobj(copyString("ciao 82", 7)), to_vnumber(82));
mapPut(map, to_vobj(copyString("ciao 83", 7)), to_vnumber(83));
mapPut(map, to_vobj(copyString("ciao 84", 7)), to_vnumber(84));
mapPut(map, to_vobj(copyString("ciao 85", 7)), to_vnumber(85));
mapPut(map, to_vobj(copyString("ciao 86", 7)), to_vnumber(86));
mapPut(map, to_vobj(copyString("ciao 87", 7)), to_vnumber(87));
mapPut(map, to_vobj(copyString("ciao 88", 7)), to_vnumber(88));
mapPut(map, to_vobj(copyString("ciao 89", 7)), to_vnumber(89));
mapPut(map, to_vobj(copyString("ciao 90", 7)), to_vnumber(90));
mapPut(map, to_vobj(copyString("ciao 91", 7)), to_vnumber(91));
mapPut(map, to_vobj(copyString("ciao 92", 7)), to_vnumber(92));
mapPut(map, to_vobj(copyString("ciao 93", 7)), to_vnumber(93));
mapPut(map, to_vobj(copyString("ciao 94", 7)), to_vnumber(94));
mapPut(map, to_vobj(copyString("ciao 95", 7)), to_vnumber(95));
mapPut(map, to_vobj(copyString("ciao 96", 7)), to_vnumber(96));
mapPut(map, to_vobj(copyString("ciao 97", 7)), to_vnumber(97));
mapPut(map, to_vobj(copyString("ciao 98", 7)), to_vnumber(98));
mapPut(map, to_vobj(copyString("ciao 99", 7)), to_vnumber(99));
mapPut(map, to_vobj(copyString("ciao 100", 8)), to_vnumber(100));
    printMap(map);
    return 0;
}
