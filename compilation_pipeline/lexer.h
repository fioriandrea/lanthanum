#ifndef lexer_h
#define lexer_h

typedef enum {
    // Single character
    TOK_COLON,
    TOK_COMMA,
    TOK_LEFT_ROUND_BRACKET,
    TOK_RIGHT_ROUND_BRACKET,
    TOK_LEFT_SQUARE_BRACKET,
    TOK_RIGHT_SQUARE_BRACKET,
    TOK_LEFT_CURLY_BRACKET,
    TOK_RIGHT_CURLY_BRACKET,
    TOK_STAR,
    TOK_MINUS,
    TOK_SLASH,
    TOK_CIRCUMFLEX,
    TOK_PERCENTAGE,
    TOK_QUESTION_MARK,

    // Single or double character
    TOK_EXCLAMATION_MARK,
    TOK_NOT_EQUAL,
    TOK_EQUAL,
    TOK_EQUAL_EQUAL,
    TOK_ARROW,
    TOK_PLUS,
    TOK_PLUS_PLUS,
    TOK_GREATER,
    TOK_GREATER_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,

    // Literals
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_NUMBER,

    // Keywords
    TOK_AND,
    TOK_OR,
    TOK_XOR,
    TOK_IF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_TRUE,
    TOK_FALSE,
    TOK_FUNC,
    TOK_RET,
    TOK_LET,
    TOK_WHILE,
    TOK_PRINT,
    TOK_NIHL,
    TOK_BREAK,
    TOK_CONTINUE,

    // special
    TOK_INDENT,
    TOK_DEDENT,
    TOK_NEW_LINE,
    TOK_ERROR,

    // eof
    TOK_EOF,
} TokenType;

typedef struct {
    char* start;
    int length;
    TokenType type;
    int line;
} Token;

#define INDENT_MAX 100

typedef struct {
    char* source;
    char* currentChar;
    char* beginningChar;
    int line;
    int indentStack[INDENT_MAX];
    int indentlen;
    int dedentCount;
    int atFirstIteration;
    int bracketDepth;
    int atEndOfLine;
} Lexer;

void initLexer(Lexer* lexer, char* src);
Token nextToken(Lexer* lexer);
void freeLexer(Lexer* lexer);

#endif
