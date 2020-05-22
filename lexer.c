#include <string.h>
#include <stdlib.h>

#include "lexer.h"

typedef struct {
    char* lexeme;
    int length;
    TokenType type;
} Keyword;

static Keyword keywords[] = {
    {"and", 3, TOK_AND},
    {"or", 2, TOK_OR},
    {"xor", 3, TOK_XOR},
    {"if", 2, TOK_IF},
    {"elif", 4, TOK_ELIF},
    {"else", 4, TOK_ELSE},
    {"true", 4, TOK_TRUE},
    {"false", 5, TOK_FALSE},
    {"func", 4, TOK_FUNC},
    {"ret", 3, TOK_RET},
    {"let", 3, TOK_LET},
    {"while", 5, TOK_WHILE},
    {"print", 5, TOK_PRINT},
    {"nihl", 4, TOK_NIHL},
    {"break", 5, TOK_BREAK},
    {"continue", 8, TOK_CONTINUE},
    {NULL, 0, TOK_ERROR}
};

void initLexer(Lexer* lexer, char* src) {
    lexer->currentChar = src;
    lexer->beginningChar = src;
    lexer->source = src;
    lexer->line = 0;
    lexer->indentStack[0] = 0;
    lexer->indentlen = 1;
    lexer->dedentCount = 0;
    lexer->atFirstIteration = 1;
    lexer->bracketDepth = 0;
    lexer->atEndOfLine = 1;
}

static Token makeGeneralToken(Lexer* lexer, TokenType type, char* start, int length) {
    if (type == TOK_NEW_LINE || type == TOK_INDENT || type == TOK_DEDENT)
        lexer->atEndOfLine = 1;
    else
        lexer->atEndOfLine = 0;
    Token tok;
    tok.start = start;
    tok.length = length;
    tok.type = type;
    tok.line = lexer->line;
    return tok;
}

static Token makeSpecial(Lexer* lexer, TokenType type, char* msg) {
    return makeGeneralToken(lexer, type, msg, strlen(msg));
}

static Token makeToken(Lexer* lexer, TokenType type) {
    return makeGeneralToken(lexer, type, lexer->beginningChar, 
            (int) (lexer->currentChar - lexer->beginningChar));
}

static Token makeError(Lexer* lexer, char* msg) {
    return makeSpecial(lexer, TOK_ERROR, msg);
}

static void sync(Lexer* lexer) {
    lexer->beginningChar = lexer->currentChar;
}

static int atEnd(Lexer* lexer) {
    return *(lexer->currentChar) == '\0';
}

static char peek(Lexer* lexer, int distance) {
    return atEnd(lexer) ? '\0' : lexer->currentChar[distance];
}

static char advance(Lexer* lexer) {
    return *(lexer->currentChar++);
}

static int isDigit(char c) {
    return c >= '0' && c <= '9';
}

static int isAlpha(char c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

static int isNonStartIdChar(char c) {
    return isAlpha(c) || isDigit(c) || c == '_';
}

static int eat(Lexer* lexer, char c) {
    if (peek(lexer, 0) == c) {
        advance(lexer);
        return 1;
    }
    return 0; 
}

static void skipEmptyLines(Lexer* lexer) {
    char* oldCurrent = lexer->currentChar;
    while (!atEnd(lexer)) {
        while (*lexer->currentChar == ' ' || *lexer->currentChar == '\t')
            lexer->currentChar++;
        if (*lexer->currentChar == '\n')
            lexer->line++;
        if (*lexer->currentChar != '\n' && *lexer->currentChar != '\0') {
            lexer->currentChar = oldCurrent;
            return;
        } else {
            lexer->currentChar++;
            oldCurrent = lexer->currentChar;
        }
    }
}

static void skipSpaces(Lexer* lexer) {
    while (!atEnd(lexer) && (*lexer->currentChar == ' ' || *lexer->currentChar == '\t'))
        lexer->currentChar++;
}

static int countSpaces(Lexer* lexer) {
    int count = 0;
    while (*lexer->currentChar == ' ' || *lexer->currentChar == '\t') {
        if (*lexer->currentChar == '\t')
            count += 4;
        else
            count++;
        advance(lexer);
    }
    return count;
}

static Token number(Lexer* lexer) {
    while (!atEnd(lexer) && isDigit(peek(lexer, 0)))
        advance(lexer);
    if (peek(lexer, 0) == '.' && isDigit(peek(lexer, 1))) {
        advance(lexer); // eat .
        while (!atEnd(lexer) && isDigit(peek(lexer, 0)))
            advance(lexer);
    }
    return makeToken(lexer, TOK_NUMBER);
}

static Token string(Lexer* lexer) {
    char quote = peek(lexer, -1);
    while (!atEnd(lexer) && peek(lexer, 0) != quote) {
        if (peek(lexer, 0) == '\n')
            lexer->line++;
        advance(lexer);
    }
    if (atEnd(lexer))
        return makeError(lexer, "unclosed string");
    advance(lexer);
    return makeToken(lexer, TOK_STRING);
}

static Token identifier(Lexer* lexer) {
    while (!atEnd(lexer) && isNonStartIdChar(peek(lexer, 0)))
        advance(lexer);
    for (Keyword* kw = keywords; kw->lexeme != NULL; kw++) {
        if (kw->length == (int) (lexer->currentChar - lexer->beginningChar)
                && memcmp(kw->lexeme, lexer->beginningChar, kw->length) == 0) {
            return makeToken(lexer, kw->type);
        } 
    }
    return makeToken(lexer, TOK_IDENTIFIER);
}

static int processNewLines(Lexer* lexer, Token* tok)  {
    // !lexer->atFirstIteration to not emit a \n if there's one (or more) \n at the beginning of the file
    if (*lexer->currentChar == '\n' && !lexer->atFirstIteration) {
        *tok = makeSpecial(lexer, TOK_NEW_LINE, "NEW_LINE");
        skipEmptyLines(lexer);
        sync(lexer);
        return 1;
    }
    return 0;
}

static int processEOF(Lexer* lexer, Token* tok) {
    if (atEnd(lexer)) {
        if (lexer->indentlen > 1) {
            lexer->indentlen--;
            *tok = makeSpecial(lexer, TOK_DEDENT, "DEDENT");
            return 1;
        }
        *tok = makeToken(lexer, TOK_EOF);
        return 1;
    }
    return 0;
}

static int processIndentation(Lexer* lexer, Token* tok) {
#define ind_stack_top lexer->indentStack[lexer->indentlen - 1]
    if (lexer->dedentCount > 0) {
        *tok = makeSpecial(lexer, TOK_DEDENT, "DEDENT");
        lexer->dedentCount--;
        return 1;
    }
    // don't indent when in brackets or when previous is not new line
    if (lexer->bracketDepth > 0 || !lexer->atEndOfLine) { 
        return 0;
    }
    char* oldcurrent = lexer->currentChar;
    int spaces = countSpaces(lexer);
    if (ind_stack_top == spaces) {
        sync(lexer); // sync to avoid having indentation in token's lexeme
        return 0;
    }
    if (ind_stack_top < spaces) {
        lexer->indentlen++;
        ind_stack_top = spaces;
        *tok = makeSpecial(lexer, TOK_INDENT, "INDENT");
    } else {
        while (spaces < ind_stack_top) {
            lexer->dedentCount++;
            lexer->indentlen--;
        }
        if (spaces == ind_stack_top) {
            *tok = makeSpecial(lexer, TOK_DEDENT, "DEDENT");
            lexer->dedentCount--;
        } else {
            *tok = makeError(lexer, "indentation error");
        }
    }
    // restores old so next time indentationToken is called, the ind_stack_top == spaces if is executed guaranteed
    lexer->currentChar = oldcurrent; 
    return 1;
#undef ind_stack_top
}

static int processBoundaries(Lexer* lexer, Token* tok) {
    if (lexer->bracketDepth > 0) {
        // ignore any whiteSpace
        skipEmptyLines(lexer);
        skipSpaces(lexer);
        sync(lexer);
        return 0;
    }
    if (!lexer->atEndOfLine) {
        return processNewLines(lexer, tok);
    } else {
        skipEmptyLines(lexer);
        sync(lexer);
        return processEOF(lexer, tok) || processIndentation(lexer, tok);
    }
}

Token nextToken(Lexer* lexer) {
    Token tok;
    if (processBoundaries(lexer, &tok)) {
        return tok;
    }
    sync(lexer);
    lexer->atFirstIteration = 0;
    tok = makeError(lexer, "unexpected character");

    char ch = '\0';
    switch (ch = advance(lexer)) {
        case ':': 
            tok = makeToken(lexer, TOK_COLON);
            break;
        case ',': 
            tok = makeToken(lexer, TOK_COMMA);
            break;
        case '(':
            tok = makeToken(lexer, TOK_LEFT_ROUND_BRACKET);
            lexer->bracketDepth++;
            break;
        case ')':
            tok = makeToken(lexer, TOK_RIGHT_ROUND_BRACKET);
            lexer->bracketDepth--;
            break;
        case '[':
            tok = makeToken(lexer, TOK_LEFT_SQUARE_BRACKET);
            lexer->bracketDepth++;
            break;
        case ']':
            tok = makeToken(lexer, TOK_RIGHT_SQUARE_BRACKET);
            lexer->bracketDepth--;
            break;
        case '{':
            tok = makeToken(lexer, TOK_LEFT_CURLY_BRACKET);
            lexer->bracketDepth++;
            break;
        case '}':
            tok = makeToken(lexer, TOK_RIGHT_CURLY_BRACKET);
            lexer->bracketDepth--;
            break;
        case '*':
            tok = makeToken(lexer, TOK_STAR);
            break;
        case '-':
            tok = makeToken(lexer, TOK_MINUS);
            break;
        case '/':
            tok = makeToken(lexer, TOK_SLASH);
            break;
        case '^':
            tok = makeToken(lexer, TOK_CIRCUMFLEX);
            break;
        case '%':
            tok = makeToken(lexer, TOK_PERCENTAGE);
            break;
        case '?':
            tok = makeToken(lexer, TOK_QUESTION_MARK);
            break;

        case '!': 
            tok = makeToken(lexer, eat(lexer, '=') ? TOK_NOT_EQUAL : TOK_EXCLAMATION_MARK); 
            break;
        case '=': 
            if (eat(lexer, '='))
                tok = makeToken(lexer, TOK_EQUAL_EQUAL);
            else if (eat(lexer, '>'))
                tok = makeToken(lexer, TOK_ARROW);
            else
                tok = makeToken(lexer, TOK_EQUAL);
            break;
        case '+': 
            tok = makeToken(lexer, eat(lexer, '+') ? TOK_PLUS_PLUS : TOK_PLUS);
            break;
        case '>': 
            tok = makeToken(lexer, eat(lexer, '=') ? TOK_GREATER_EQUAL : TOK_GREATER);
            break;
        case '<': 
            tok = makeToken(lexer, eat(lexer, '=') ? TOK_LESS_EQUAL : TOK_LESS);
            break;
        case '\'':
        case '"':
            tok = string(lexer);
            break;
        default:
            if (isDigit(ch)) {
                tok = number(lexer);
            } else if (isAlpha(ch)) {
                tok = identifier(lexer);
            }
    }
    skipSpaces(lexer); // not skipEmptyLines because you'd skip eventual \n after token
    lexer->atEndOfLine = 0;
    return tok;
}

void freeLexer(Lexer* lexer) {
    free(lexer->source);
}
