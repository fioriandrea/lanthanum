#include <stdio.h>
#include <stdlib.h>

#include "datastructs/chunk.h"
#include "services/asm_printer.h"
#include "services/token_printer.h"
#include "vm.h"
#include "lexer.h"

int main(int argc, char **argv) {
    Lexer lex;
    Token tok;
    initLexer(&lex, "if");
    do {
        tok = nextToken(&lex);
        printToken(tok);
    } while (tok.type != TOK_EOF);

    return 0;
}
