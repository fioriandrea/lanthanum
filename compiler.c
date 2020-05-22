#include <stdlib.h>
#include <stdio.h>

#include "compiler.h"
#include "standardtypes.h"
#include "./datastructs/chunk.h"

#include "./services/token_printer.h"
#define TRACE_TOKENS 

/*
program -> statement* EOF
statement -> print | let | if | while | func | ret | break | continue | expressionStat
print -> 'print' expression NEW_LINE
let -> 'let' IDENTIFIER '=' expression NEW_LINE
if -> 'if' expression block ('elif' expression block)* ('else' block)?
while -> 'while' expression block
func -> 'func' IDENTIFIER '(' paramList ')' block
ret -> 'ret' (expression)? NEW_LINE
break -> 'break' NEW_LINE
continue -> 'continue' NEW_LINE
expressionStat -> expression NEW_LINE
block -> INDENT statement* DEDENT

expression -> comma
comma -> nonCommaExpr (',' nonCommaExpr)*
nonCommaExpr -> assign
assign -> ternary '=' expression
ternary -> logicalSum ('?' ternary : ternary)?
logicalSum -> and (('or' | 'xor') and)*
and -> equal ('and' equal)*
equal -> comparison (('==' | '!=') comparison)*
comparison -> sum (('<' | '<=' | '>' | '>=') sum)*
sum -> mult (('+' | '-' | '++') mult)*
mult -> pow (('*' | '/' | '%') pow)*
pow -> unary | unary '^' pow
unary -> call | ('-' | '!') unary
call -> functionCall | indexing
functionCall -> primary '(' argList ')'
argList -> nonCommaExpr (',' nonCommaExpr)* | ''
indexing -> primary '[' expression ']'
primary -> basic | '(' expression ')' | array | map
basic -> STRING | NUMBER | TRUE | FALSE | NIHL | IDENTIFIER
array -> '[' arrayList ']'
arrayList -> nonCommaExpr (',' nonCommaExpr)* | ''
map -> '{' mapList '}'
mapList -> mapElement (',' mapElement) | ''
mapElement -> nonCommaExpr '=>' nonCommaExpr
*/

static void expression(Compiler* compiler);

static Chunk* compilingChunk(Compiler* compiler) {
    return compiler->compilingChunk;
}

static void error(Compiler* compiler, Token tok, char* message) {
    if (compiler->panic)
        return;
    fprintf(stderr, "error ");
    if (tok.type == TOK_EOF) {
        fprintf(stderr, "[at end]: ");
    } else {
        fprintf(stderr, "[at %d]: ", tok.line);
    }
    if (tok.type != TOK_ERROR) {
        fprintf(stderr, "at '%.*s', ", tok.length, tok.start);
    }
    fprintf(stderr, "%s\n", message);
    compiler->hadError = 1;
    compiler->panic = 1;
}

static void errorAtCurrent(Compiler* compiler, char* message) {
    error(compiler, compiler->current, message);
}

static void errorAtPrevious(Compiler* compiler, char* message) {
    error(compiler, compiler->previous, message);
}

static TokenType previousTokenType(Compiler* compiler) {
    return compiler->previous.type;
}

static TokenType currentTokenType(Compiler* compiler) {
    return compiler->current.type;
}

static void advance(Compiler* compiler) {
   compiler->previous = compiler->current;
   for (;;) {
        compiler->current = nextToken(&compiler->lexer);
#ifdef TRACE_TOKENS
        printf("\n...\n");
        printToken(compiler->current);
        printf("...\n");
#endif
        if (compiler->current.type != TOK_ERROR)
            break;
        errorAtCurrent(compiler, "");
   } 
}

static int check(Compiler* compiler, TokenType type) {
    return compiler->current.type == type;
}   

static int eat(Compiler* compiler, TokenType type) {
    if (check(compiler, type)) {
        advance(compiler);
        return 1;
    }
    return 0;
}

static void eatError(Compiler* compiler, TokenType type, char* msg) {
    if (!eat(compiler, type)) {
        errorAtCurrent(compiler, msg);
    }
}

void initCompiler(Compiler* compiler, char* source) {
    initLexer(&compiler->lexer, source);
    compiler->hadError = 0;
    compiler->panic = 0;
}

static void emitByte(Compiler* compiler, uint8_t byte) {
    printToken(compiler->current);
    writeChunk(compilingChunk(compiler), byte, compiler->current.line);
}

static void emitConstant(Compiler* compiler, Value val) {
    int index = writeConstant(compilingChunk(compiler), val);
    emitByte(compiler, OP_CONST);
    emitByte(compiler, index);
}

static void emitRet(Compiler* compiler) {
    emitByte(compiler, OP_RET);
}

static void emitUnary(Compiler* compiler, TokenType operator) {
    switch (operator) {
        case TOK_PLUS: break; // NO_OP
        case TOK_MINUS: emitByte(compiler, OP_NEGATE); break;
        case TOK_EXCLAMATION_MARK: emitByte(compiler, OP_NOT); break;
    }
}

static void emitBinary(Compiler* compiler, TokenType operator) {
    switch (operator) {
        case TOK_PLUS: emitByte(compiler, OP_ADD); break;
        case TOK_MINUS: emitByte(compiler, OP_SUB); break;
        case TOK_STAR: emitByte(compiler, OP_MUL); break;
        case TOK_SLASH: emitByte(compiler, OP_DIV); break;
        case TOK_PERCENTAGE: emitByte(compiler, OP_MOD); break;
        case TOK_CIRCUMFLEX: emitByte(compiler, OP_POW); break;
    }
}

static void numberExpression(Compiler* compiler) {
    Value val = strtod(compiler->current.start, NULL);
    emitConstant(compiler, val);
    advance(compiler);
}

static void stringExpression(Compiler* compiler) {

}

static void basicExpression(Compiler* compiler) {
    switch (currentTokenType(compiler)) {
        case TOK_STRING:
            stringExpression(compiler);
            break;
        case TOK_NUMBER:
            numberExpression(compiler);
            break;
        case TOK_TRUE:
            emitByte(compiler, OP_CONST_TRUE);
            advance(compiler);
            break;
        case TOK_FALSE:
            emitByte(compiler, OP_CONST_FALSE);
            advance(compiler);
            break;
        case TOK_NIHL:
            emitByte(compiler, OP_CONST_NIHL);
            advance(compiler);
            break;
        default:
            errorAtCurrent(compiler, "unexpected token");
    }
}

static void groupingExpression(Compiler* compiler) {
    advance(compiler); // skip (
    expression(compiler);
    eatError(compiler, TOK_RIGHT_ROUND_BRACKET, "missing ')' after grouping expression");
}

static void primaryExpression(Compiler* compiler) {
    switch (currentTokenType(compiler)) {
        case TOK_LEFT_ROUND_BRACKET:
            groupingExpression(compiler);
            break;
        default:
            basicExpression(compiler);
            break;
    }
}

static void unaryExpression(Compiler* compiler) {
    TokenType operator;
    if (check(compiler, TOK_MINUS) || check(compiler, TOK_PLUS) || check(compiler, TOK_EXCLAMATION_MARK)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        unaryExpression(compiler);
        emitUnary(compiler, operator);
    } else {
        primaryExpression(compiler);
    }
}

static void powExpression(Compiler* compiler) {
    TokenType operator;
    unaryExpression(compiler);
    if (check(compiler, TOK_CIRCUMFLEX)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        powExpression(compiler);
        emitBinary(compiler, operator);
    }
}

static void multExpression(Compiler* compiler) {
    powExpression(compiler);
    TokenType operator;
    while (check(compiler, TOK_STAR) || check(compiler, TOK_SLASH) || check(compiler, TOK_PERCENTAGE)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        powExpression(compiler);
        emitBinary(compiler, operator);
    }
}

static void addExpression(Compiler* compiler) {
    multExpression(compiler);
    TokenType operator;
    while (check(compiler, TOK_PLUS) || check(compiler, TOK_MINUS)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        multExpression(compiler);
        emitBinary(compiler, operator);
    }
}

static void expression(Compiler* compiler) {
    return addExpression(compiler); 
}

int compile(Compiler* compiler, Chunk* chunk) {
    compiler->compilingChunk = chunk;
    advance(compiler);
    expression(compiler);
    eatError(compiler, TOK_NEW_LINE, "expected new line");
    eatError(compiler, TOK_EOF, "expected EOF");
    emitRet(compiler);
    return !compiler->hadError;
}

void freeCompiler(Compiler* compiler) {
    freeLexer(&compiler->lexer);
}
