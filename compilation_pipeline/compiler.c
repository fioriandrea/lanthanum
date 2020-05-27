#include <stdlib.h>
#include <stdio.h>

#include "compiler.h"
#include "../datastructs/chunk.h"
#include "../datastructs/value.h"
#include "../datastructs/object.h"

#include "../debug/token_printer.h"
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

#define standard_binary_expression(name, next, condition) \
    static void name(Compiler* compiler, int canAssign) { \
        TokenType operator; \
        next(compiler, canAssign); \
        while (condition) { \
            operator = currentTokenType(compiler); \
            advance(compiler); \
            next(compiler, 0); \
            emitBinary(compiler, operator); \
        } \
    }       

static void expression(Compiler* compiler);
static void statement(Compiler* compiler);

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
        advance(compiler);
    }
}

void initCompiler(Compiler* compiler) {
    compiler->hadError = 0;
    compiler->panic = 0;
    compiler->collector = NULL;
    compiler->scope.count = 0;
    compiler->scope.depth = 0;
}

static void emitByte(Compiler* compiler, uint8_t byte) {
    writeChunk(compiler->collector, compilingChunk(compiler), byte, compiler->current.line);
}

static void emitConstant(Compiler* compiler, Value val) {
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), OP_CONST_LONG, OP_CONST, val, compiler->current.line);
}

static void emitGlobalDecl(Compiler* compiler, Token identifier) {
    ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
    Value name = to_vobj(strname);
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), OP_GLOBAL_DECL_LONG, OP_GLOBAL_DECL, name, compiler->current.line);
}

static void emitGlobalGet(Compiler* compiler, Token identifier) {
    ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
    Value name = to_vobj(strname);
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), OP_GLOBAL_GET_LONG, OP_GLOBAL_GET, name, compiler->previous.line);
}

static void emitGlobalSet(Compiler* compiler, Token identifier) {
    ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
    Value name = to_vobj(strname);
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), OP_GLOBAL_SET_LONG, OP_GLOBAL_SET, name, compiler->previous.line);
}

static int identifiersEqual(Token id1, Token id2) {
    return id1.length == id2.length &&
        memcmp(id1.start, id2.start, id1.length) == 0;
}

static int indexLocal(Compiler* compiler, Token identifier) {
    for (int i = compiler->scope.count - 1; i >= 0; i--) {   
        Local* local = &compiler->scope.locals[i];                  
        if (identifiersEqual(identifier, local->name)) {           
            return i;                                           
        }                                                     
    }
    return -1; 
}

static void emitLocalGet(Compiler* compiler, Token identifier) {
    int index = indexLocal(compiler, identifier);
    if (index < 0) {
        emitGlobalGet(compiler, identifier);
    } else {
        if (compiler->scope.locals[index].depth < 0) { 
            errorAtCurrent(compiler, "cannot read local variable in its own initializer");
        }
        ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
        Value name = to_vobj(strname);
        writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_LOCAL_GET_LONG, OP_LOCAL_GET, (uint16_t) index, compiler->previous.line);
    }
}

static void emitLocalSet(Compiler* compiler, Token identifier) {
    int index = indexLocal(compiler, identifier);
    if (index < 0) {
        emitGlobalSet(compiler, identifier);
    } else {
        ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
        Value name = to_vobj(strname);
        writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_LOCAL_SET_LONG, OP_LOCAL_SET, (uint16_t) index, compiler->previous.line);
    }
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
        case TOK_EQUAL_EQUAL: emitByte(compiler, OP_EQUAL); break;
        case TOK_NOT_EQUAL: emitByte(compiler, OP_NOT_EQUAL); break;
        case TOK_LESS: emitByte(compiler, OP_LESS); break;
        case TOK_LESS_EQUAL: emitByte(compiler, OP_LESS_EQUAL); break;
        case TOK_GREATER: emitByte(compiler, OP_GREATER); break;
        case TOK_GREATER_EQUAL: emitByte(compiler, OP_GREATER_EQUAL); break;
        case TOK_PLUS_PLUS: emitByte(compiler, OP_CONCAT); break;
    }
}

static int alreadyDeclaredLocal(Scope* scope, Token identifier) {
    for (int i = scope->count - 1; i >= 0; i--) {
        if (scope->locals[i].depth < scope->depth)
            return 0;
        if (identifiersEqual(scope->locals[i].name, identifier))
            return 1;
    }
    return 0; // unreachable
}

static void declareLocal(Compiler* compiler, Token identifier) {
    Scope* scope = &compiler->scope;
    if (alreadyDeclaredLocal(scope, identifier)) {
        errorAtCurrent(compiler, "variable with this name already declared in this scope");
        return;
    }
    Local* local = &scope->locals[scope->count];
    local->name = identifier;
    local->depth = -1;
    scope->count++;
}

static void defineLocal(Compiler* compiler, Token identifier) {
    int index = indexLocal(compiler, identifier);
    // todo: check if index >= 0 ?
    compiler->scope.locals[index].depth = compiler->scope.depth;
}

static void numberExpression(Compiler* compiler) {
    double val = strtod(compiler->current.start, NULL);
    emitConstant(compiler, to_vnumber(val));
    advance(compiler);
}

static void stringExpression(Compiler* compiler) {
    ObjString* string = copyString(compiler->collector, compiler->current.start + 1, compiler->current.length - 2);
    emitConstant(compiler, to_vobj(string));
    advance(compiler);
}

static void identifierExpression(Compiler* compiler, int canAssign) {
    Token identifier = compiler->current;
    advance(compiler);
    void (*emitGet)(Compiler*, Token) = NULL;
    void (*emitSet)(Compiler*, Token) = NULL;
    if (compiler->scope.depth > 0) { // locals realm
        emitGet = &emitLocalGet;
        emitSet = &emitLocalSet;
    } else {
        emitGet = &emitGlobalGet;
        emitSet = &emitGlobalSet;
    }

    if (canAssign && eat(compiler, TOK_EQUAL)) {
        expression(compiler);
        (*emitSet)(compiler, identifier);
    } else {
        (*emitGet)(compiler, identifier);
    }
}

static void basicExpression(Compiler* compiler, int canAssign) {
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
        case TOK_IDENTIFIER:
            identifierExpression(compiler, canAssign);
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

static void primaryExpression(Compiler* compiler, int canAssign) {
    switch (currentTokenType(compiler)) {
        case TOK_LEFT_ROUND_BRACKET:
            groupingExpression(compiler);
            break;
        default:
            basicExpression(compiler, canAssign);
            break;
    }
}

static void unaryExpression(Compiler* compiler, int canAssign) {
    TokenType operator;
    if (check(compiler, TOK_MINUS) || check(compiler, TOK_PLUS) || check(compiler, TOK_EXCLAMATION_MARK)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        unaryExpression(compiler, canAssign);
        emitUnary(compiler, operator);
    } else {
        primaryExpression(compiler, canAssign);
    }
}

static void powExpression(Compiler* compiler, int canAssign) {
    TokenType operator;
    unaryExpression(compiler, canAssign);
    if (check(compiler, TOK_CIRCUMFLEX)) {
        operator = currentTokenType(compiler);
        advance(compiler);
        powExpression(compiler, 0);
        emitBinary(compiler, operator);
    }
}

standard_binary_expression(multExpression, powExpression,
        check(compiler, TOK_STAR) || check(compiler, TOK_SLASH) || check(compiler, TOK_PERCENTAGE))

standard_binary_expression(addExpression, multExpression,
        check(compiler, TOK_PLUS) || check(compiler, TOK_MINUS) || check(compiler, TOK_PLUS_PLUS))

standard_binary_expression(comparisonExpression, addExpression,
        check(compiler, TOK_LESS) || check(compiler, TOK_LESS_EQUAL)
        || check (compiler, TOK_GREATER) || check(compiler, TOK_GREATER_EQUAL))

standard_binary_expression(equalExpression, comparisonExpression, 
        check(compiler, TOK_EQUAL_EQUAL) || check(compiler, TOK_NOT_EQUAL))

    static void nonCommaExpression(Compiler* compiler) {
        equalExpression(compiler, 1);
    }

static void commaExpression(Compiler* compiler) {
    nonCommaExpression(compiler);
    while (eat(compiler, TOK_COMMA)) {
        emitByte(compiler, OP_POP);
        nonCommaExpression(compiler);
    }
}

static void expression(Compiler* compiler) {
    return commaExpression(compiler); 
}

static void expressionStat(Compiler* compiler) {
    expression(compiler);
    emitByte(compiler, OP_POP);
    eatError(compiler, TOK_NEW_LINE, "expected new line at end of statement");
}

static void printStat(Compiler* compiler) {
    advance(compiler); // skip 'print'
    expression(compiler);
    emitByte(compiler, OP_PRINT);
    emitByte(compiler, OP_POP);
    eatError(compiler, TOK_NEW_LINE, "expected new line at end of statement");
}

static void letStat(Compiler* compiler) {
    advance(compiler); // skip 'let'
    eatError(compiler, TOK_IDENTIFIER, "expected identifier after \"let\"");
    Token identifier = compiler->previous;
    if (compiler->scope.depth > 0) {
        declareLocal(compiler, identifier);
    }
    if (eat(compiler, TOK_EQUAL)) {
        expression(compiler);
    } else {
        emitByte(compiler, OP_CONST_NIHL);
    }
    if (compiler->scope.depth == 0) {
        emitGlobalDecl(compiler, identifier);
    } else {
        defineLocal(compiler, identifier);
    }
    eatError(compiler, TOK_NEW_LINE, "expected new line at end of statement");
}

static void block(Compiler* compiler) {
    while (currentTokenType(compiler) != TOK_EOF && currentTokenType(compiler) != TOK_DEDENT) {
        statement(compiler);
    }
    eatError(compiler, TOK_DEDENT, "missing dedent to close block");
    //eatError(compiler, TOK_NEW_LINE, "expect new line after dedent");
}

static void startScope(Compiler* compiler) {
    compiler->scope.depth++;
}

static void endScope(Compiler* compiler) {
    Scope* scope = &compiler->scope;
    while (scope->count > 0 && scope->locals[scope->count - 1].depth == scope->depth) {
        emitByte(compiler, OP_POP);
        scope->count--;
    }
    compiler->scope.depth--;
}

static void blockStat(Compiler* compiler) {
    advance(compiler);
    //eatError(compiler, TOK_NEW_LINE, "expect new line after indent");
    startScope(compiler);
    block(compiler);
    endScope(compiler);
}

static void statement(Compiler* compiler) {
    switch (currentTokenType(compiler)) {
        case TOK_LET:
            letStat(compiler);
            break;
        case TOK_PRINT:
            printStat(compiler);
            break;
        case TOK_INDENT:
            blockStat(compiler);
            break;
        default:
            expressionStat(compiler);
            break;
    }
}

static void statementList(Compiler* compiler) {
    while (!check(compiler, TOK_EOF)) {
        statement(compiler);
    }
}

int compile(Compiler* compiler, Collector* collector, Chunk* chunk, char* source) {
    initCompiler(compiler);
    initLexer(&compiler->lexer, source);
    compiler->collector = collector;
    compiler->compilingChunk = chunk;

    advance(compiler);
    statementList(compiler);
    emitRet(compiler);
    freeLexer(&compiler->lexer);
    return !compiler->hadError;
}

void freeCompiler(Compiler* compiler) {
}
