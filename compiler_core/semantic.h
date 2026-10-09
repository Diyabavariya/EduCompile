#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

#define MAX_SYMBOLS 128
#define MAX_TAC 256

typedef struct {
    char name[MAX_LEXEME];
    char type[16];
    int declared;
    int used;
    double value;
    int has_value;
} Symbol;

typedef struct {
    char result[MAX_LEXEME];
    char arg1[MAX_LEXEME];
    char op[8];
    char arg2[MAX_LEXEME];
    int has_op;
    int has_arg2;
} TACInstr;

typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int symbol_count;

    /* Unoptimized TAC (before constant folding) */
    TACInstr tac_before[MAX_TAC];
    int tac_before_count;

    /* Optimized TAC (after constant folding) */
    TACInstr tac[MAX_TAC];
    int tac_count;

    int temp_count;
    char errors[MAX_ERRORS][256];
    int error_count;
} SemanticResult;

SemanticResult analyze_and_generate(ASTNode* root);

#endif
