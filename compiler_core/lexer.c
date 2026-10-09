#include "lexer.h"

const char* token_type_str(TokenType t) {
    switch (t) {
        case TOK_NUMBER:     return "NUMBER";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
        case TOK_ASSIGN:     return "OPERATOR";
        case TOK_LPAREN:
        case TOK_RPAREN:
        case TOK_LBRACE:
        case TOK_RBRACE:
        case TOK_SEMICOLON:
        case TOK_COMMA:      return "DELIMITER";
        case TOK_INT:
        case TOK_FLOAT:
        case TOK_DOUBLE:
        case TOK_RETURN:     return "KEYWORD";
        case TOK_EOF:        return "EOF";
        case TOK_INVALID:    return "INVALID";
        default:             return "UNKNOWN";
    }
}

static void add_error(LexerResult* res, int line, int col, const char* msg) {
    if (res->error_count >= MAX_ERRORS) return;
    snprintf(res->errors[res->error_count], 256,
             "[Error] Line %d, Col %d: Lexical Error - %s", line, col, msg);
    res->error_count++;
}

static void add_token(LexerResult* res, TokenType type, const char* lex,
                      int line, int col) {
    if (res->count >= MAX_TOKENS) return;
    Token* t = &res->tokens[res->count++];
    t->type = type;
    strncpy(t->lexeme, lex, MAX_LEXEME - 1);
    t->lexeme[MAX_LEXEME - 1] = '\0';
    t->line = line;
    t->column = col;
}

LexerResult tokenize(const char* source) {
    LexerResult res;
    memset(&res, 0, sizeof(res));

    int i = 0;
    int line = 1;
    int col = 1;
    int len = (int)strlen(source);

    while (i < len) {
        // Skip whitespace
        if (isspace(source[i])) {
            if (source[i] == '\n') {
                line++;
                col = 1;
            } else if (source[i] == '\t') {
                col += 4; // Standard tab stop
            } else {
                col++;
            }
            i++;
            continue;
        }

        // Skip // comments
        if (source[i] == '/' && i + 1 < len && source[i + 1] == '/') {
            while (i < len && source[i] != '\n') {
                i++;
                col++;
            }
            continue;
        }

        int start_col = col;

        // Number
        if (isdigit(source[i]) || (source[i] == '.' && i + 1 < len && isdigit(source[i + 1]))) {
            char buf[MAX_LEXEME];
            int j = 0;
            while (i < len && (isdigit(source[i]) || source[i] == '.') && j < MAX_LEXEME - 1) {
                buf[j++] = source[i++];
                col++;
            }
            buf[j] = '\0';
            add_token(&res, TOK_NUMBER, buf, line, start_col);
            continue;
        }

        // Identifier or Keyword
        if (isalpha(source[i]) || source[i] == '_') {
            char buf[MAX_LEXEME];
            int j = 0;
            while (i < len && (isalnum(source[i]) || source[i] == '_') && j < MAX_LEXEME - 1) {
                buf[j++] = source[i++];
                col++;
            }
            buf[j] = '\0';

            // Check for keywords
            TokenType type = TOK_IDENTIFIER;
            if (strcmp(buf, "int") == 0) type = TOK_INT;
            else if (strcmp(buf, "float") == 0) type = TOK_FLOAT;
            else if (strcmp(buf, "double") == 0) type = TOK_DOUBLE;
            else if (strcmp(buf, "return") == 0) type = TOK_RETURN;

            add_token(&res, type, buf, line, start_col);
            continue;
        }

        // Single character tokens
        char ch = source[i];
        char single[2] = {ch, '\0'};
        TokenType type = TOK_INVALID;

        switch (ch) {
            case '+': type = TOK_PLUS; break;
            case '-': type = TOK_MINUS; break;
            case '*': type = TOK_MULTIPLY; break;
            case '/': type = TOK_DIVIDE; break;
            case '(': type = TOK_LPAREN; break;
            case ')': type = TOK_RPAREN; break;
            case '{': type = TOK_LBRACE; break;
            case '}': type = TOK_RBRACE; break;
            case ',': type = TOK_COMMA; break;
            case '=': type = TOK_ASSIGN; break;
            case ';': type = TOK_SEMICOLON; break;
            default:
                type = TOK_INVALID;
                char msg[64];
                snprintf(msg, sizeof(msg), "Invalid character '%c'", ch);
                add_error(&res, line, col, msg);
                break;
        }

        add_token(&res, type, single, line, start_col);
        i++;
        col++;
    }

    add_token(&res, TOK_EOF, "", line, col);
    return res;
}
