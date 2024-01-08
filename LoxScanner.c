#include <stdio.h>
#include <string.h>
#include "common.h"
#include "LoxScanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} LoxScanner;

//global scanner to process the input Lox code
LoxScanner scanner;


void initScanner(const char* sourceCode) {
    //keeping track of place within the input, allows us to create substrings and process tokens
    scanner.start = sourceCode;
    scanner.current = sourceCode;
    scanner.line = 1;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}


static bool isAtEnd() {
    return *scanner.current == '\0';
}

//increment through source code input
static char advanceToken() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek(){
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

//make sure the 
static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static LoxToken makeToken(TokenType type) {
    LoxToken token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static LoxToken errorToken(const char* message) {
    LoxToken token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advanceToken();
                break;
            case '/':
                if (peekNext() == '/') {
                // A comment goes until the end of the line.
                    while(peek() != '\n' && !isAtEnd()) advanceToken();
                    } 
                else {
                    return;
                    }
                break;
            case '\n':
                scanner.line++;
                advanceToken();
                break;
            default:
                return;
}
}
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    else {
    return TOKEN_IDENTIFIER;
}
}


//check the input for any keywords, checking if they are Lox keywords
static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        //unique case, check 'for', 'false' and 'fun'(function declaration)
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        //unique case, check for this and true
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
        }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
}
    return TOKEN_IDENTIFIER;
}


static LoxToken handleIdentifier() {
    while (isAlpha(peek()) || isDigit(peek())) advanceToken();
    return makeToken(identifierType());
}


static LoxToken handleNumber() {
    while (isDigit(peek())) advanceToken();
    // handle decimals, make sure the next char is a digit 
    if (peek() == '.' && isDigit(peekNext())) {
    // consume the . symbol and continue through input until the number is finished
    advanceToken();
    while (isDigit(peek())) advanceToken();
}
    return makeToken(TOKEN_NUMBER);
}

static LoxToken handleStr() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advanceToken();
}
    if (isAtEnd()) return errorToken("Unterminated string.");
    // handle the ending quote of the string
    advanceToken();
    return makeToken(TOKEN_STRING);
}



//iterate through the tokens by each character to add create each scanned Lox Token
LoxToken scanToken(){
    //work through all the white space, comments, new lines, etc before scanning the tokens
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advanceToken();
    if (isAlpha(c)) return handleIdentifier();
    if (isDigit(c)) return handleNumber();
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!':
        return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
            case '>': 
                return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        
        case '"': return handleStr();
        default:
            return errorToken("Unexpected character.");
    }
}
