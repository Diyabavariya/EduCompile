#include "parser.h"

static Token* tokens;
static int pos;
static int token_count;
static ParserResult* result;

/* Nodes live in a static pool so child pointers remain valid after parse() returns */
static ASTNode node_pool[MAX_AST_NODES];
static int node_pool_count = 0;

static ASTNode* new_node(NodeType type, const char* value, int line, int col) {
    if (node_pool_count >= MAX_AST_NODES) return NULL;
    ASTNode* n = &node_pool[node_pool_count++];
    memset(n, 0, sizeof(ASTNode));
    n->type = type;
    if (value) {
        strncpy(n->value, value, MAX_LEXEME - 1);
        n->value[MAX_LEXEME - 1] = '\0';
    }
    n->line = line;
    n->column = col;
    return n;
}

static void add_child(ASTNode* parent, ASTNode* child) {
    if (parent && child && parent->child_count < MAX_CHILDREN) {
        parent->children[parent->child_count++] = child;
    }
}

static void add_parse_error(const char* msg, Token* t) {
    if (result->error_count >= MAX_ERRORS) return;
    if (t) {
        snprintf(result->errors[result->error_count], 256,
                 "[Error] Line %d, Col %d: Syntax Error - %s", t->line, t->column, msg);
    } else {
        snprintf(result->errors[result->error_count], 256, "[Error] Syntax Error - %s", msg);
    }
    result->error_count++;
    result->has_error = 1;
}

static Token* current() {
    if (pos < token_count) return &tokens[pos];
    return &tokens[token_count - 1];
}

static Token* advance() {
    Token* t = current();
    if (pos < token_count - 1) pos++;
    return t;
}

static int match(TokenType type) {
    return current()->type == type;
}

static int match_type() {
    return match(TOK_INT) || match(TOK_FLOAT) || match(TOK_DOUBLE);
}

static Token* expect(TokenType type, const char* msg) {
    if (match(type)) return advance();
    add_parse_error(msg, current());
    return NULL;
}

// Forward declarations
static ASTNode* parse_expression();
static ASTNode* parse_term();
static ASTNode* parse_factor();
static ASTNode* parse_statement();
static ASTNode* parse_declaration();
static ASTNode* parse_function();

static ASTNode* parse_declaration() {
    Token* type = advance(); // int or float
    Token* id = expect(TOK_IDENTIFIER, "Expected identifier after type");
    ASTNode* decl = new_node(NODE_DECLARATION, id->lexeme, type->line, type->column);

    // Optional assignment: int x = 5;
    if (match(TOK_ASSIGN)) {
        advance();
        ASTNode* expr = parse_expression();
        add_child(decl, expr);
    }

    expect(TOK_SEMICOLON, "Expected ';' after declaration");
    return decl;
}

static ASTNode* parse_function() {
    Token* ret_type = advance(); // int or float
    Token* name = expect(TOK_IDENTIFIER, "Expected function name");
    ASTNode* func = new_node(NODE_FUNCTION, name->lexeme, ret_type->line, ret_type->column);

    expect(TOK_LPAREN, "Expected '(' after function name");
    ASTNode* params = new_node(NODE_PARAM_LIST, "params", current()->line, current()->column);
    add_child(func, params);

    if (!match(TOK_RPAREN)) {
        do {
            if (match_type()) {
                Token* p_type = advance();
                Token* p_name = expect(TOK_IDENTIFIER, "Expected parameter name");
                ASTNode* p = new_node(NODE_PARAM, p_name->lexeme, p_type->line, p_type->column);
                add_child(params, p);
            } else {
                break;
            }
            if (match(TOK_COMMA)) advance();
            else break;
        } while (1);
    }
    expect(TOK_RPAREN, "Expected ')' after parameters");

    expect(TOK_LBRACE, "Expected '{' to start function body");
    while (!match(TOK_RBRACE) && !match(TOK_EOF)) {
        ASTNode* stmt = parse_statement();
        if (stmt) add_child(func, stmt);
    }
    expect(TOK_RBRACE, "Expected '}' after function body");

    return func;
}

static ASTNode* parse_factor() {
    Token* t = current();

    if (match(TOK_NUMBER)) {
        advance();
        return new_node(NODE_NUMBER, t->lexeme, t->line, t->column);
    }
    if (match(TOK_IDENTIFIER)) {
        advance();
        return new_node(NODE_IDENTIFIER, t->lexeme, t->line, t->column);
    }
    if (match(TOK_LPAREN)) {
        advance();
        ASTNode* expr = parse_expression();
        expect(TOK_RPAREN, "Expected ')'");
        return expr;
    }
    if (match(TOK_MINUS)) {
        Token* op = advance();
        ASTNode* operand = parse_factor();
        ASTNode* node = new_node(NODE_UNARY, "-", op->line, op->column);
        add_child(node, operand);
        return node;
    }

    add_parse_error("Unexpected token in factor", t);
    advance(); // recover
    return new_node(NODE_NUMBER, "0", t->line, t->column);
}

static ASTNode* parse_term() {
    ASTNode* left = parse_factor();
    while (match(TOK_MULTIPLY) || match(TOK_DIVIDE)) {
        Token* op = advance();
        ASTNode* right = parse_factor();
        ASTNode* node = new_node(NODE_BINARY, op->lexeme, op->line, op->column);
        add_child(node, left);
        add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode* parse_expression() {
    ASTNode* left = parse_term();
    while (match(TOK_PLUS) || match(TOK_MINUS)) {
        Token* op = advance();
        ASTNode* right = parse_term();
        ASTNode* node = new_node(NODE_BINARY, op->lexeme, op->line, op->column);
        add_child(node, left);
        add_child(node, right);
        left = node;
    }
    return left;
}

static ASTNode* parse_statement() {
    if (match(TOK_RETURN)) {
        Token* ret = advance();
        ASTNode* expr = parse_expression();
        if (match(TOK_SEMICOLON)) advance();
        ASTNode* node = new_node(NODE_RETURN, "return", ret->line, ret->column);
        add_child(node, expr);
        return node;
    }

    if (match_type()) {
        // Check if it's a function or declaration
        if (pos + 2 < token_count && tokens[pos+1].type == TOK_IDENTIFIER && tokens[pos+2].type == TOK_LPAREN) {
            return parse_function();
        }
        return parse_declaration();
    }

    if (match(TOK_IDENTIFIER) && pos + 1 < token_count && tokens[pos + 1].type == TOK_ASSIGN) {
        Token* id = advance();
        advance(); // =
        ASTNode* value = parse_expression();
        if (match(TOK_SEMICOLON)) advance();
        ASTNode* node = new_node(NODE_ASSIGN, id->lexeme, id->line, id->column);
        add_child(node, value);
        return node;
    } else {
        ASTNode* expr = parse_expression();
        if (match(TOK_SEMICOLON)) advance();
        return expr;
    }
}

ParserResult parse(LexerResult* lex) {
    ParserResult res;
    memset(&res, 0, sizeof(res));
    result = &res;
    tokens = lex->tokens;
    token_count = lex->count;
    pos = 0;
    node_pool_count = 0;   /* reset static pool */

    ASTNode* program = new_node(NODE_PROGRAM, "program", 1, 1);
    res.root = program;

    // Create a shadow Parse Tree root in the JSON
    // (For simplicity in this educational tool, we will generate PT data by adding
    // intermediate rule nodes to the JSON serialization logic when in PT mode)

    while (!match(TOK_EOF)) {
        if (match(TOK_INVALID)) {
            advance();
            continue;
        }
        ASTNode* stmt = parse_statement();
        if (stmt) add_child(program, stmt);

        // Panic recovery
        if (res.has_error) {
            while (!match(TOK_SEMICOLON) && !match(TOK_LBRACE) && !match(TOK_RBRACE) && !match(TOK_EOF)) advance();
            if (match(TOK_SEMICOLON)) advance();
            res.has_error = 0;
        }
    }
    return res;
}

void ast_to_string(ASTNode* node, char* buf, int* pos, int max, int indent) {
    if (!node || *pos >= max - 100) return;

    for (int i = 0; i < indent; i++) {
        *pos += snprintf(buf + *pos, max - *pos, "  ");
    }

    switch (node->type) {
        case NODE_NUMBER:
            *pos += snprintf(buf + *pos, max - *pos, "Number(%s)\n", node->value);
            break;
        case NODE_IDENTIFIER:
            *pos += snprintf(buf + *pos, max - *pos, "Identifier(%s)\n", node->value);
            break;
        case NODE_BINARY:
            *pos += snprintf(buf + *pos, max - *pos, "BinaryOp('%s')\n", node->value);
            break;
        case NODE_UNARY:
            *pos += snprintf(buf + *pos, max - *pos, "UnaryOp('%s')\n", node->value);
            break;
        case NODE_ASSIGN:
            *pos += snprintf(buf + *pos, max - *pos, "Assignment(%s =)\n", node->value);
            break;
        case NODE_DECLARATION:
            *pos += snprintf(buf + *pos, max - *pos, "Declaration(%s)\n", node->value);
            break;
        case NODE_FUNCTION:
            *pos += snprintf(buf + *pos, max - *pos, "Function(%s)\n", node->value);
            break;
        case NODE_RETURN:
            *pos += snprintf(buf + *pos, max - *pos, "Return\n");
            break;
        case NODE_PARAM_LIST:
            *pos += snprintf(buf + *pos, max - *pos, "Parameters\n");
            break;
        case NODE_PARAM:
            *pos += snprintf(buf + *pos, max - *pos, "Param(%s)\n", node->value);
            break;
        case NODE_PROGRAM:
            *pos += snprintf(buf + *pos, max - *pos, "Program\n");
            break;
        case NODE_PT_EXPR:
            *pos += snprintf(buf + *pos, max - *pos, "[Expression]\n");
            break;
        case NODE_PT_TERM:
            *pos += snprintf(buf + *pos, max - *pos, "[Term]\n");
            break;
        case NODE_PT_FACTOR:
            *pos += snprintf(buf + *pos, max - *pos, "[Factor]\n");
            break;
        case NODE_PT_STMT:
            *pos += snprintf(buf + *pos, max - *pos, "[Statement]\n");
            break;
    }

    for (int i = 0; i < node->child_count; i++) {
        ast_to_string(node->children[i], buf, pos, max, indent + 1);
    }
}

static const char* node_type_label(NodeType t) {
    switch(t) {
        case NODE_PROGRAM: return "Program";
        case NODE_FUNCTION: return "Function";
        case NODE_DECLARATION: return "Declaration";
        case NODE_ASSIGN: return "Assignment";
        case NODE_BINARY: return "BinaryOp";
        case NODE_UNARY: return "UnaryOp";
        case NODE_NUMBER: return "Literal";
        case NODE_IDENTIFIER: return "Identifier";
        case NODE_RETURN: return "Return";
        case NODE_PARAM_LIST: return "Params";
        case NODE_PARAM: return "Param";
        case NODE_PT_EXPR: return "Expression";
        case NODE_PT_TERM: return "Term";
        case NODE_PT_FACTOR: return "Factor";
        case NODE_PT_STMT: return "Statement";
        default: return "Node";
    }
}

void serialize_ast_json(ASTNode* node, char* buf, int* p, int max) {
    if (!node || *p >= max - 500) {
        *p += snprintf(buf + *p, max - *p, "null");
        return;
    }

    *p += snprintf(buf + *p, max - *p, "{\"type\": \"%s\", \"value\": \"%s\", \"children\": [",
                  node_type_label(node->type), node->value);

    for (int i = 0; i < node->child_count; i++) {
        serialize_ast_json(node->children[i], buf, p, max);
        if (i < node->child_count - 1) *p += snprintf(buf + *p, max - *p, ",");
    }

    *p += snprintf(buf + *p, max - *p, "]}");
}

void serialize_pt_json(ASTNode* node, char* buf, int* p, int max) {
    if (!node || *p >= max - 500) {
        *p += snprintf(buf + *p, max - *p, "null");
        return;
    }

    const char* wrapper = NULL;
    if (node->type == NODE_BINARY || node->type == NODE_UNARY) wrapper = "Expression";
    else if (node->type == NODE_NUMBER || node->type == NODE_IDENTIFIER) wrapper = "Factor";
    else if (node->type == NODE_ASSIGN || node->type == NODE_DECLARATION) wrapper = "Statement";

    if (wrapper) {
        *p += snprintf(buf + *p, max - *p, "{\"type\": \"%s\", \"value\": \"\", \"children\": [", wrapper);
    }

    *p += snprintf(buf + *p, max - *p, "{\"type\": \"%s\", \"value\": \"%s\", \"children\": [",
                  node_type_label(node->type), node->value);

    for (int i = 0; i < node->child_count; i++) {
        serialize_pt_json(node->children[i], buf, p, max);
        if (i < node->child_count - 1) *p += snprintf(buf + *p, max - *p, ",");
    }

    *p += snprintf(buf + *p, max - *p, "]}");
    if (wrapper) *p += snprintf(buf + *p, max - *p, "]}");
}
