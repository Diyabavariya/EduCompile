#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SOURCE 8192
#define MAX_JSON 65536

// Very simple JSON string escaper
static void json_escape(const char* src, char* dest, int max) {
    int j = 0;
    for (int i = 0; src[i] && j < max - 2; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            dest[j++] = '\\';
            dest[j++] = src[i];
        } else if (src[i] == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else if (src[i] == '\t') {
            dest[j++] = '\\';
            dest[j++] = 't';
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

int main(int argc, char* argv[]) {
    char source[MAX_SOURCE];
    memset(source, 0, sizeof(source));

    // Read source from stdin
    size_t n = fread(source, 1, MAX_SOURCE - 1, stdin);
    source[n] = '\0';

    if (n == 0) {
        printf("{\"success\":false,\"error\":\"Empty source code\"}\n");
        return 1;
    }

    // Phase 1: Lexical
    LexerResult lex = tokenize(source);

    // Phase 2: Syntax
    ParserResult par = parse(&lex);

    // Phase 3: Semantic + Codegen
    SemanticResult sem = {0};
    if (par.root && lex.error_count == 0) {
        sem = analyze_and_generate(par.root);
    }

    // Build JSON
    char json[MAX_JSON];
    int pos = 0;

    pos += snprintf(json + pos, MAX_JSON - pos, "{\n");
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"success\": %s,\n",
                    (lex.error_count == 0 && par.error_count == 0 && sem.error_count == 0) ? "true" : "false");

    // Tokens
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"tokens\": [\n");
    for (int i = 0; i < lex.count; i++) {
        if (lex.tokens[i].type == TOK_EOF) continue;
        char esc[MAX_LEXEME * 2];
        json_escape(lex.tokens[i].lexeme, esc, sizeof(esc));
        pos += snprintf(json + pos, MAX_JSON - pos,
            "    {\"type\": \"%s\", \"value\": \"%s\", \"line\": %d, \"column\": %d}%s\n",
            token_type_str(lex.tokens[i].type), esc,
            lex.tokens[i].line, lex.tokens[i].column,
            (i < lex.count - 2) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // Lexical errors
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"lexical_errors\": [\n");
    for (int i = 0; i < lex.error_count; i++) {
        char esc[512];
        json_escape(lex.errors[i], esc, sizeof(esc));
        pos += snprintf(json + pos, MAX_JSON - pos, "    \"%s\"%s\n",
                        esc, (i < lex.error_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // AST string
    char ast_buf[8192];
    int ast_pos = 0;
    memset(ast_buf, 0, sizeof(ast_buf));
    if (par.root) {
        ast_to_string(par.root, ast_buf, &ast_pos, sizeof(ast_buf), 0);
    }
    char ast_esc[16384];
    json_escape(ast_buf, ast_esc, sizeof(ast_esc));
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"ast\": \"%s\",\n", ast_esc);

    // Visual JSON AST for graphical rendering
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"ast_json\": ");
    serialize_ast_json(par.root, json, &pos, MAX_JSON);
    pos += snprintf(json + pos, MAX_JSON - pos, ",\n");

    // Visual JSON Parse Tree (PT)
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"pt_json\": ");
    void serialize_pt_json(ASTNode* node, char* buf, int* p, int max);
    serialize_pt_json(par.root, json, &pos, MAX_JSON);
    pos += snprintf(json + pos, MAX_JSON - pos, ",\n");

    // Syntax errors
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"syntax_errors\": [\n");
    for (int i = 0; i < par.error_count; i++) {
        char esc[512];
        json_escape(par.errors[i], esc, sizeof(esc));
        pos += snprintf(json + pos, MAX_JSON - pos, "    \"%s\"%s\n",
                        esc, (i < par.error_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // Symbol table
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"symbol_table\": [\n");
    for (int i = 0; i < sem.symbol_count; i++) {
        char valbuf[32] = "null";
        if (sem.symbols[i].has_value) {
            snprintf(valbuf, sizeof(valbuf), "%g", sem.symbols[i].value);
        }
        pos += snprintf(json + pos, MAX_JSON - pos,
            "    {\"name\": \"%s\", \"type\": \"%s\", \"used\": %s, \"value\": %s}%s\n",
            sem.symbols[i].name, sem.symbols[i].type,
            sem.symbols[i].used ? "true" : "false",
            valbuf,
            (i < sem.symbol_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // TAC Before Optimization (no constant folding)
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"tac_before\": [\n");
    for (int i = 0; i < sem.tac_before_count; i++) {
        TACInstr* t = &sem.tac_before[i];
        char e_res[MAX_LEXEME*2], e_a1[MAX_LEXEME*2], e_op[16], e_a2[MAX_LEXEME*2];
        json_escape(t->result, e_res, sizeof(e_res));
        json_escape(t->arg1, e_a1, sizeof(e_a1));
        json_escape(t->op, e_op, sizeof(e_op));
        json_escape(t->arg2, e_a2, sizeof(e_a2));

        pos += snprintf(json + pos, MAX_JSON - pos,
            "    {\"result\": \"%s\", \"arg1\": \"%s\", \"op\": \"%s\", \"arg2\": \"%s\", \"has_op\": %s, \"has_arg2\": %s}%s\n",
            e_res, e_a1, t->has_op ? e_op : "", t->has_arg2 ? e_a2 : "",
            t->has_op ? "true" : "false", t->has_arg2 ? "true" : "false",
            (i < sem.tac_before_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // TAC After Optimization (with constant folding)
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"tac\": [\n");
    for (int i = 0; i < sem.tac_count; i++) {
        TACInstr* t = &sem.tac[i];
        char e_res[MAX_LEXEME*2], e_a1[MAX_LEXEME*2], e_op[16], e_a2[MAX_LEXEME*2];
        json_escape(t->result, e_res, sizeof(e_res));
        json_escape(t->arg1, e_a1, sizeof(e_a1));
        json_escape(t->op, e_op, sizeof(e_op));
        json_escape(t->arg2, e_a2, sizeof(e_a2));

        pos += snprintf(json + pos, MAX_JSON - pos,
            "    {\"result\": \"%s\", \"arg1\": \"%s\", \"op\": \"%s\", \"arg2\": \"%s\", \"has_op\": %s, \"has_arg2\": %s}%s\n",
            e_res, e_a1, t->has_op ? e_op : "", t->has_arg2 ? e_a2 : "",
            t->has_op ? "true" : "false", t->has_arg2 ? "true" : "false",
            (i < sem.tac_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // Semantic errors
    pos += snprintf(json + pos, MAX_JSON - pos, "  \"semantic_errors\": [\n");
    for (int i = 0; i < sem.error_count; i++) {
        char esc[512];
        json_escape(sem.errors[i], esc, sizeof(esc));
        pos += snprintf(json + pos, MAX_JSON - pos, "    \"%s\"%s\n",
                        esc, (i < sem.error_count - 1) ? "," : "");
    }
    pos += snprintf(json + pos, MAX_JSON - pos, "  ],\n");

    // Summary
    pos += snprintf(json + pos, MAX_JSON - pos,
        "  \"summary\": {\n"
        "    \"lexical_errors\": %d,\n"
        "    \"syntax_errors\": %d,\n"
        "    \"semantic_errors\": %d,\n"
        "    \"token_count\": %d,\n"
        "    \"tac_before_count\": %d,\n"
        "    \"tac_count\": %d,\n"
        "    \"variable_count\": %d\n"
        "  }\n",
        lex.error_count, par.error_count, sem.error_count,
        lex.count > 0 ? lex.count - 1 : 0,
        sem.tac_before_count, sem.tac_count, sem.symbol_count);

    pos += snprintf(json + pos, MAX_JSON - pos, "}\n");

    printf("%s", json);
    return 0;
}
