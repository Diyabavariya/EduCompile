#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_AST_NODES 512
#define MAX_CHILDREN 8

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_DECLARATION,
    NODE_ASSIGN,
    NODE_BINARY,
    NODE_UNARY,
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_RETURN,
    NODE_PARAM_LIST,
    NODE_PARAM,
    // Parse Tree specific nodes
    NODE_PT_EXPR,
    NODE_PT_TERM,
    NODE_PT_FACTOR,
    NODE_PT_STMT
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char value[MAX_LEXEME];   // number string or identifier or operator
    int line;
    int column;
    struct ASTNode* children[MAX_CHILDREN];
    int child_count;
} ASTNode;

typedef struct {
    ASTNode nodes[MAX_AST_NODES];
    int node_count;
    ASTNode* root;
    char errors[MAX_ERRORS][256];
    int error_count;
    int has_error;
} ParserResult;

ParserResult parse(LexerResult* lex);

// Helper to print AST as indented text (for JSON we will serialize later)
void ast_to_string(ASTNode* node, char* buf, int* pos, int max, int indent);
void serialize_ast_json(ASTNode* node, char* buf, int* p, int max);

#endif
