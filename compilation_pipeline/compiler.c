#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "compiler.h"
#include "../datastructs/chunk.h"
#include "../datastructs/value.h"
#include "../util.h"
#include "../datastructs/object.h"
#include "../debug/debug_switches.h"

#define MAX_BRANCHES 200

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
static void nonCommaExpression(Compiler* compiler);
static void statement(Compiler* compiler);

static inline Chunk* compilingChunk(Compiler* compiler) {
    return compiler->scope->function->chunk;
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
}

static void emitByte(Compiler* compiler, uint8_t byte) {
    writeChunk(compiler->collector, compilingChunk(compiler), byte, compiler->current.line);
}

static inline void emit_addressable_at_current(Compiler* compiler, OpCode longCode, OpCode shortCode, Value value) {
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), longCode, shortCode, value, compiler->current.line);
}

static inline void emit_addressable_at_previous(Compiler* compiler, OpCode longCode, OpCode shortCode, Value value) {
    writeAddressableInstruction(compiler->collector, compilingChunk(compiler), longCode, shortCode, value, compiler->previous.line);
}

static void emitConstant(Compiler* compiler, Value val) {
    emit_addressable_at_current(compiler, OP_CONST_LONG, OP_CONST, val);
}

static void emitClosure(Compiler* compiler, Scope* scope, ObjFunction* function) {
    emit_addressable_at_current(compiler, OP_CLOSURE_LONG, OP_CLOSURE, to_vobj(function));
    for (int i = 0; i < function->upvalueCount; i++) {
        Upvalue* upvalue = &scope->upvalues[i];
        emitByte(compiler, upvalue->ownedAbove);
        emitByte(compiler, upvalue->index);
    }
}

static void emitGlobalDecl(Compiler* compiler, Token identifier) {
    ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
    Value name = to_vobj(strname);
    emit_addressable_at_current(compiler, OP_GLOBAL_DECL_LONG, OP_GLOBAL_DECL, name);
}

static void emitGlobalGet(Compiler* compiler, Value name, int index) {
    emit_addressable_at_previous(compiler, OP_GLOBAL_GET_LONG, OP_GLOBAL_GET, name);
}

static void emitGlobalSet(Compiler* compiler, Value name, int index) {
    emit_addressable_at_previous(compiler, OP_GLOBAL_SET_LONG, OP_GLOBAL_SET, name);
}

static int identifiersEqual(Token id1, Token id2) {
    return id1.length == id2.length &&
        memcmp(id1.start, id2.start, id1.length) == 0;
}

static int indexLocal(Scope* scope, Token identifier) {
    for (int i = scope->localsCount - 1; i >= 0; i--) {   
        Local* local = &scope->locals[i];                  
        if (identifiersEqual(identifier, local->name)) {
            return i;                                           
        }                                                     
    }
    return -1; 
}

static int addUpvalue(Scope* scope, int index, int ownedAbove) {
    int upvalueCount = scope->function->upvalueCount;
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &scope->upvalues[i];
        if (upvalue->index == index && upvalue->ownedAbove == ownedAbove)
            return i;
    }
    scope->upvalues[upvalueCount].index = index;
    scope->upvalues[upvalueCount].ownedAbove = ownedAbove;
    scope->function->upvalueCount++;
    return upvalueCount;
}

static int indexUpvalue(Scope* scope, Token identifier) {
    if (scope->enclosing == NULL)
        return -1;
    int index = indexLocal(scope->enclosing, identifier);
    if (index >= 0) {
        scope->enclosing->locals[index].isCaptured = 1;
        return addUpvalue(scope, index, 0);
    } else {
        index = indexUpvalue(scope->enclosing, identifier);
        if (index >= 0) {
            return addUpvalue(scope, index, 1);
        }
    }
    return -1;
}

static void emitUpvalueGet(Compiler* compiler, Value name, int index) {
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_UPVALUE_GET_LONG, OP_UPVALUE_GET, (uint16_t) index, compiler->previous.line);
}

static void emitUpvalueSet(Compiler* compiler, Value name, int index) {
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_UPVALUE_SET_LONG, OP_UPVALUE_SET, (uint16_t) index, compiler->previous.line);
}

static void emitLocalGet(Compiler* compiler, Value name, int index) {
    if (compiler->scope->locals[index].depth < 0) { 
        errorAtCurrent(compiler, "cannot read local variable in its own initializer");
    }
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_LOCAL_GET_LONG, OP_LOCAL_GET, (uint16_t) index, compiler->previous.line);
}

static void emitLocalSet(Compiler* compiler, Value name, int index) {
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_LOCAL_SET_LONG, OP_LOCAL_SET, (uint16_t) index, compiler->previous.line);
}

static void emitRet(Compiler* compiler) {
    emitByte(compiler, OP_CONST_NIHL);
    emitByte(compiler, OP_RET);
}

static void emitUnary(Compiler* compiler, TokenType operator) {
    switch (operator) {
        case TOK_PLUS: break; // NO_OP
        case TOK_MINUS: emitByte(compiler, OP_NEGATE); break;
        case TOK_EXCLAMATION_MARK: emitByte(compiler, OP_NOT); break;
    }
}

static int emitJump(Compiler* compiler, OpCode opcode) {
    emitByte(compiler, opcode);
    emitByte(compiler, 0x00);
    emitByte(compiler, 0x00);
    return compilingChunk(compiler)->count - 3;
}

static int patchJump(Compiler* compiler, int address) {
    int newarg = compilingChunk(compiler)->count - address;
    if (newarg > UINT16_MAX) {                                    
        errorAtCurrent(compiler, "branch too big");                     
    } 
    SplittedLong sl = split_long((uint16_t) newarg);
    compilingChunk(compiler)->code[address + 1] = sl.b0;
    compilingChunk(compiler)->code[address + 2] = sl.b1;  
}

static void emitJumpBack(Compiler* compiler, int address) {
    int offset = compilingChunk(compiler)->count - address;
    if (offset > UINT16_MAX) {
        errorAtCurrent(compiler, "loop body too big");
    }
    SplittedLong sl = split_long((uint16_t) offset);
    emitByte(compiler, OP_JUMP_BACK);
    emitByte(compiler, sl.b0);
    emitByte(compiler, sl.b1);
}

static void emitArrayLiteral(Compiler* compiler, int count) {
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_ARRAY_LONG, OP_ARRAY, (uint16_t) count, compiler->previous.line);
}

static void emitDictionaryLiteral(Compiler* compiler, int count) {
    writeVariableSizeOp(compiler->collector, compilingChunk(compiler), OP_DICT_LONG, OP_DICT, (uint16_t) count, compiler->previous.line);
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
        case TOK_XOR: emitByte(compiler, OP_XOR); break;
    }
}

static void initSkipLoopList(SkipLoopList* sll) {
    sll->breakCount = 0;
    sll->continueCount = 0;
}

static void pushScope(Compiler* compiler, Scope* scope, ObjString* name) {
    scope->enclosing = compiler->scope;
    initSkipLoopList(&scope->skipLoopList);
    scope->depth = 0;
    scope->localsCount = 0;
    scope->loopDepth = 0;
    scope->function = newFunction(compiler->collector);
    scope->function->name = name;
    compiler->scope = scope;
}

static ObjFunction* popScope(Compiler* compiler) {
    emitRet(compiler);
    ObjFunction* function = compiler->scope->function;
    compiler->scope = compiler->scope->enclosing;
#ifdef PRINT_CODE
    printf("FUNCTION CODE:\n");
    printChunk(function->chunk, function->name == NULL ? "main code" : function->name->chars);
    printf("\n");
#endif
    return function;
}

static int alreadyDeclaredLocal(Scope* scope, Token identifier) {
    for (int i = scope->localsCount - 1; i >= 0; i--) {
        if (scope->locals[i].depth < scope->depth)
            return 0;
        if (identifiersEqual(scope->locals[i].name, identifier))
            return 1;
    }
    return 0; // unreachable
}

static void declareLocal(Compiler* compiler, Token identifier) {
    Scope* scope = compiler->scope;
    if (alreadyDeclaredLocal(scope, identifier)) {
        errorAtCurrent(compiler, "variable with this name already declared in this scope");
        return;
    }
    Local* local = &scope->locals[scope->localsCount];
    local->name = identifier;
    local->isCaptured = 0;
    local->depth = -1;
    scope->localsCount++;
}

static void defineLocal(Compiler* compiler, Token identifier) {
    int index = indexLocal(compiler->scope, identifier);
    // todo: check if index >= 0 ?
    compiler->scope->locals[index].depth = compiler->scope->depth;
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
    ObjString* strname = copyString(compiler->collector, identifier.start, identifier.length);
    Value name = to_vobj(strname);
    advance(compiler);
    void (*emitGet)(Compiler*, Value, int) = NULL;
    void (*emitSet)(Compiler*, Value, int) = NULL;
    int index = -1;
    if (compiler->scope->depth > 0 && (index = indexLocal(compiler->scope, identifier)) >= 0) { // locals realm
        emitGet = &emitLocalGet;
        emitSet = &emitLocalSet;
    } else if ((index = indexUpvalue(compiler->scope, identifier)) >= 0) {
        emitGet = &emitUpvalueGet;
        emitSet = &emitUpvalueSet;
    } else {
        emitGet = &emitGlobalGet;
        emitSet = &emitGlobalSet;
    }

    if (canAssign && eat(compiler, TOK_EQUAL)) {
        expression(compiler);
        (*emitSet)(compiler, name, index);
    } else {
        (*emitGet)(compiler, name, index);
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

static void arrayExpression(Compiler* compiler) {
    advance(compiler);
    int count = 0;
    if (eat(compiler, TOK_RIGHT_SQUARE_BRACKET)) {
        emitArrayLiteral(compiler, 0);
        return;
    }
    do {
        nonCommaExpression(compiler);
        count++;
    } while (eat(compiler, TOK_COMMA));
    eatError(compiler, TOK_RIGHT_SQUARE_BRACKET, "expected \"]\" after array literal");
    emitArrayLiteral(compiler, count);
}

static void dictionaryExpression(Compiler* compiler) {
    advance(compiler);
    int count = 0;
    if (eat(compiler, TOK_RIGHT_CURLY_BRACKET)) {
        emitDictionaryLiteral(compiler, count);
        return;
    }
    do {
        nonCommaExpression(compiler);
        eatError(compiler, TOK_ARROW, "expected \"=>\" after dictionary key");
        nonCommaExpression(compiler);
        count++;
    } while (eat(compiler, TOK_COMMA));
    eatError(compiler, TOK_RIGHT_CURLY_BRACKET, "expected \"}\" after dictionary literal");
    emitDictionaryLiteral(compiler, count);
}

static void primaryExpression(Compiler* compiler, int canAssign) {
    switch (currentTokenType(compiler)) {
        case TOK_LEFT_ROUND_BRACKET:
            groupingExpression(compiler);
            break;
        case TOK_LEFT_SQUARE_BRACKET:
            arrayExpression(compiler);
            break;
        case TOK_LEFT_CURLY_BRACKET:
            dictionaryExpression(compiler);
            break;
        default:
            basicExpression(compiler, canAssign);
            break;
    }
}

static uint8_t argList(Compiler* compiler) {
    uint8_t argCount = 0;
    if (!check(compiler, TOK_RIGHT_ROUND_BRACKET)) {
        do {
            argCount++;
            nonCommaExpression(compiler);
        } while (eat(compiler, TOK_COMMA));
    }
    eatError(compiler, TOK_RIGHT_ROUND_BRACKET, "expect \")\" after function arguments");
    if (argCount >= UINT8_MAX) {                          
        errorAtCurrent(compiler, "function arguments limit exceeded");
    }  
    return argCount;
}

static void callExpression(Compiler* compiler, int canAssign) {
    primaryExpression(compiler, canAssign);    
    while (check(compiler, TOK_LEFT_ROUND_BRACKET) || check(compiler, TOK_LEFT_SQUARE_BRACKET)) {
        advance(compiler);
        switch (previousTokenType(compiler)) {
            case TOK_LEFT_ROUND_BRACKET:
                {
                    uint8_t argCount = argList(compiler);
                    emitByte(compiler, OP_CALL);
                    emitByte(compiler, argCount);
                    break;
                }
            case TOK_LEFT_SQUARE_BRACKET:
                {
                    expression(compiler);
                    eatError(compiler, TOK_RIGHT_SQUARE_BRACKET, "expected \"]\" after indexing expression");
                    if (canAssign && eat(compiler, TOK_EQUAL)) {
                        expression(compiler);
                        emitByte(compiler, OP_INDEXING_SET);
                    } else {
                        emitByte(compiler, OP_INDEXING_GET);
                    }
                    break;
                }
        }
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
        callExpression(compiler, canAssign);
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

    static void andExpression(Compiler* compiler, int canAssign) {
        int jumpAddresses[MAX_BRANCHES];
        int jumpAddressesPointer = 0;

        equalExpression(compiler, canAssign);
        while (eat(compiler, TOK_AND)) {
            jumpAddresses[jumpAddressesPointer++] = emitJump(compiler, OP_JUMP_IF_FALSE);
            emitByte(compiler, OP_POP);
            equalExpression(compiler, 0);
        }

        for (int i = 0; i < jumpAddressesPointer; i++)
            patchJump(compiler, jumpAddresses[i]);
    }

static void orExpression(Compiler* compiler, int canAssign) {
    int jumpAddresses[MAX_BRANCHES];
    int jumpAddressesPointer = 0;
    while (eat(compiler, TOK_OR)) {
        jumpAddresses[jumpAddressesPointer++] = emitJump(compiler, OP_JUMP_IF_TRUE);
        emitByte(compiler, OP_POP);
        andExpression(compiler, 0);
    }

    for (int i = 0; i < jumpAddressesPointer; i++)
        patchJump(compiler, jumpAddresses[i]);
}

static void logicalSumExpression(Compiler* compiler, int canAssign) {
    TokenType operator;
    andExpression(compiler, canAssign);
    while (check(compiler, TOK_OR) || check(compiler, TOK_XOR)) {
        operator = currentTokenType(compiler);
        if (operator == TOK_XOR) {
            advance(compiler);
            andExpression(compiler, 0);
            emitBinary(compiler, TOK_XOR);
        } else {
            orExpression(compiler, 0);
        }
    }
}

static void ternaryExpression(Compiler* compiler, int canAssign) {
    logicalSumExpression(compiler, canAssign);
    if (eat(compiler, TOK_QUESTION_MARK)) {
        int skipfirst = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP);
        expression(compiler);
        int skipsecond = emitJump(compiler, OP_JUMP);
        patchJump(compiler, skipfirst);
        emitByte(compiler, OP_POP);
        eatError(compiler, TOK_COLON, "expected \":\" inside ternary expression");
        expression(compiler);
        patchJump(compiler, skipsecond);
    }
}

static void nonCommaExpression(Compiler* compiler) {
    ternaryExpression(compiler, 1);
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

static void block(Compiler* compiler) {
    while (currentTokenType(compiler) != TOK_EOF && currentTokenType(compiler) != TOK_DEDENT) {
        statement(compiler);
    }
    eatError(compiler, TOK_DEDENT, "missing dedent to close block");
}

static void startScope(Compiler* compiler) {
    compiler->scope->depth++;
}

static void endScope(Compiler* compiler) {
    Scope* scope = compiler->scope;
    while (scope->localsCount > 0 && scope->locals[scope->localsCount - 1].depth == scope->depth) {
        if (scope->locals[scope->localsCount - 1].isCaptured)
            emitByte(compiler, OP_CLOSE_UPVALUE);
        else 
            emitByte(compiler, OP_POP);
        scope->localsCount--;
    }
    compiler->scope->depth--;
}

static void blockStat(Compiler* compiler) {
    advance(compiler);
    startScope(compiler);
    block(compiler);
    endScope(compiler);
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
    eatError(compiler, TOK_NEW_LINE, "expected new line at end of statement");
}

static void letStat(Compiler* compiler) {
    advance(compiler); // skip 'let'
    eatError(compiler, TOK_IDENTIFIER, "expected identifier after \"let\"");
    Token identifier = compiler->previous;
    if (compiler->scope->depth > 0) {
        declareLocal(compiler, identifier);
    }
    if (eat(compiler, TOK_EQUAL)) {
        expression(compiler);
    } else {
        emitByte(compiler, OP_CONST_NIHL);
    }
    if (compiler->scope->depth == 0) {
        emitGlobalDecl(compiler, identifier);
    } else {
        defineLocal(compiler, identifier);
    }
    eatError(compiler, TOK_NEW_LINE, "expected new line at end of statement");
}

static void parseFunctionDeclaration(Compiler* compiler, Token name) {
    Scope scope;
    pushScope(compiler, &scope, copyString(compiler->collector, name.start, name.length));
    startScope(compiler);
    eatError(compiler, TOK_LEFT_ROUND_BRACKET, "expected \"(\" before function parameters");
    if (!check(compiler, TOK_RIGHT_ROUND_BRACKET)) {
        do {
            compiler->scope->function->arity++;
            if (compiler->scope->function->arity >= UINT8_MAX)
                errorAtCurrent(compiler, "maximum number of function parameters exceeded");
            eatError(compiler, TOK_IDENTIFIER, "expected identifier inside function's \"()\"");
            Token identifier = compiler->previous;
            declareLocal(compiler, identifier);
            defineLocal(compiler, identifier);
        } while (eat(compiler, TOK_COMMA));
    }
    eatError(compiler, TOK_RIGHT_ROUND_BRACKET, "expected \")\" after function parameters");
    eatError(compiler, TOK_NEW_LINE, "expected new line after function parameters");
    if (!check(compiler, TOK_INDENT))
        errorAtCurrent(compiler, "expected indent after function parameters");
    advance(compiler);
    block(compiler);
    ObjFunction* function = popScope(compiler);
    emitClosure(compiler, &scope, function);
}

static void funcStat(Compiler* compiler) {
    advance(compiler); // skip 'func'
    eatError(compiler, TOK_IDENTIFIER, "expected function name after \"func\"");
    Token identifier = compiler->previous;

    if (compiler->scope->depth > 0) {
        declareLocal(compiler, identifier);
        defineLocal(compiler, identifier);
    }
    parseFunctionDeclaration(compiler, identifier);
    if (compiler->scope->depth == 0) {
        emitGlobalDecl(compiler, identifier);
    } 
}

static void ifStat(Compiler* compiler) {
    int jumpAddresses[MAX_BRANCHES];
    int jumpAddressesPointer = 0;

    advance(compiler); // skip if
    expression(compiler);
    eatError(compiler, TOK_NEW_LINE, "expected new line after if condition");
    int jumpif = emitJump(compiler, OP_JUMP_IF_FALSE);
    emitByte(compiler, OP_POP);
    if (!check(compiler, TOK_INDENT))
        errorAtCurrent(compiler, "expect indent after if");
    blockStat(compiler);
    jumpAddresses[jumpAddressesPointer++] = emitJump(compiler, OP_JUMP);
    patchJump(compiler, jumpif); 
    emitByte(compiler, OP_POP);

    while (eat(compiler, TOK_ELIF)) {
        expression(compiler);
        eatError(compiler, TOK_NEW_LINE, "expected new line after elif condition");
        int jumpelif = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP);
        if (!check(compiler, TOK_INDENT))
            errorAtCurrent(compiler, "expect indent after elif");
        blockStat(compiler);
        jumpAddresses[jumpAddressesPointer++] = emitJump(compiler, OP_JUMP);
        patchJump(compiler, jumpelif); 
        emitByte(compiler, OP_POP);
    }

    if (eat(compiler, TOK_ELSE)) {
        eatError(compiler, TOK_NEW_LINE, "expected new line after else");
        if (!check(compiler, TOK_INDENT))
            errorAtCurrent(compiler, "expect indent after else");
        blockStat(compiler);
    }

    for (int i = 0; i < jumpAddressesPointer; i++)
        patchJump(compiler, jumpAddresses[i]);
}

static inline SkipLoopList* get_skip_loop_list(Compiler* compiler) {
    return &compiler->scope->skipLoopList;
}

static void pushBreak(Compiler* compiler, int breakAddress) {
    SkipLoopList* sll = get_skip_loop_list(compiler); 
    sll->breakAddresses[sll->breakCount++] = breakAddress;
}

static int popBreak(Compiler* compiler) {
    SkipLoopList* sll = get_skip_loop_list(compiler); 
    return sll->breakAddresses[--sll->breakCount];
}

static void patchBreak(Compiler* compiler) {
    SkipLoopList* sll = get_skip_loop_list(compiler); 
    if (sll->breakCount > 0)
        patchJump(compiler, popBreak(compiler));
}

static void emitBreak(Compiler* compiler) {
    int breakAddress = emitJump(compiler, OP_JUMP);
    pushBreak(compiler, breakAddress);
}

static void breakStat(Compiler* compiler) {
    if (compiler->scope->loopDepth <= 0) {
        errorAtCurrent(compiler, "cannot use \"break\" outside of a loop");
    }
    advance(compiler); // skip break
    eatError(compiler, TOK_NEW_LINE, "expected new line after \"break\"");
    emitBreak(compiler); 
}

static void whileStat(Compiler* compiler) {
    compiler->scope->loopDepth++;
    advance(compiler); // skip while
    int jumpBackAddress = compilingChunk(compiler)->count; 
    expression(compiler);
    eatError(compiler, TOK_NEW_LINE, "expected new line after while condition");
    int jumpwhile = emitJump(compiler, OP_JUMP_IF_FALSE);
    emitByte(compiler, OP_POP);
    if (!check(compiler, TOK_INDENT))
        errorAtCurrent(compiler, "expect indent after while");
    blockStat(compiler);
    emitJumpBack(compiler, jumpBackAddress);
    patchJump(compiler, jumpwhile); 
    emitByte(compiler, OP_POP);
    patchBreak(compiler);
    compiler->scope->loopDepth--;
}

static void retStat(Compiler* compiler) {
    advance(compiler);
    if (check(compiler, TOK_NEW_LINE) || check(compiler, TOK_EOF)) {
        emitRet(compiler);
    } else {
        expression(compiler);
        emitByte(compiler, OP_RET);
    }
    if (!check(compiler, TOK_NEW_LINE) && !check(compiler, TOK_EOF))
        errorAtCurrent(compiler, "unexpected token after ret statement");
    else
        advance(compiler);
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
        case TOK_IF:
            ifStat(compiler);
            break;
        case TOK_WHILE:
            whileStat(compiler);
            break;
        case TOK_FUNC:
            funcStat(compiler);
            break;
        case TOK_RET:
            retStat(compiler);
            break;
        case TOK_BREAK:
            breakStat(compiler);
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

ObjFunction* compile(Compiler* compiler, Collector* collector, char* source) {
    initCompiler(compiler);
    initLexer(&compiler->lexer, source);
    compiler->collector = collector;
    Scope startingScope;
    startingScope.enclosing = NULL;
    startingScope.depth = 0;
    startingScope.localsCount = 0;
    startingScope.function = newFunction(compiler->collector);
    compiler->scope = &startingScope;

    advance(compiler);
    statementList(compiler);
    freeLexer(&compiler->lexer);
    return !compiler->hadError ? popScope(compiler) : NULL;
}

void freeCompiler(Compiler* compiler) {
}
