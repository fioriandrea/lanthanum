#include "compiler.h"

#include "./services/token_printer.h"

void initCompiler(Compiler* compiler, char* source) {
    initLexer(&compiler->lexer, source);
}

void compile(Compiler* compiler, Chunk* chunk) {
    // temp code
    Token tok;
    do {
        tok = nextToken(&compiler->lexer);
        printToken(tok);
    } while (tok.type != TOK_EOF);
}

void freeCompiler(Compiler* compiler) {
    freeLexer(&compiler->lexer);
}
