#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 512
#define MAX_LEXEME 128
#define MAX_ERRORS 64

typedef enum {
    TOK_NUMBER,
    TOK_IDENTIFIER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_ASSIGN,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_INT,
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_RETURN,
    TOK_EOF,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME];
    int line;
    int column;
} Token;

typedef struct {
    Token tokens[MAX_TOKENS];
    int count;
    char errors[MAX_ERRORS][256];
    int error_count;
} LexerResult;

const char* token_type_str(TokenType t);
LexerResult tokenize(const char* source);

#endif
