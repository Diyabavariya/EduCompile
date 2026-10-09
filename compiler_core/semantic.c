#include "semantic.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static SemanticResult* sem;
static char temp_buf[32];
static int fold_mode = 0; /* 0 = no folding (before), 1 = folding (after) */

static char* new_temp() {
    sem->temp_count++;
    snprintf(temp_buf, sizeof(temp_buf), "t%d", sem->temp_count);
    return temp_buf;
}

static int is_number_str(const char* s) {
    if (!s || !*s) return 0;
    char* end;
    strtod(s, &end);
    return *end == '\0';
}

static Symbol* lookup(const char* name) {
    for (int i = 0; i < sem->symbol_count; i++) {
        if (strcmp(sem->symbols[i].name, name) == 0)
            return &sem->symbols[i];
    }
    return NULL;
}

static void declare(const char* name) {
    Symbol* s = lookup(name);
    if (s) {
        s->declared = 1;
        return;
    }
    if (sem->symbol_count >= MAX_SYMBOLS) return;
    s = &sem->symbols[sem->symbol_count++];
    strncpy(s->name, name, MAX_LEXEME - 1);
    s->name[MAX_LEXEME - 1] = '\0';
    strcpy(s->type, "number");
    s->declared = 1;
    s->used = 0;
    s->has_value = 0;
}

static void mark_used(const char* name) {
    Symbol* s = lookup(name);
    if (s) s->used = 1;
}

static void add_sem_error(const char* msg) {
    if (sem->error_count >= MAX_ERRORS) return;
    snprintf(sem->errors[sem->error_count], 256, "[Error] Semantic Error - %s", msg);
    sem->error_count++;
}

static void emit_to(TACInstr* arr, int* count, const char* result, const char* arg1, const char* op, const char* arg2) {
    if (*count >= MAX_TAC) return;
    TACInstr* t = &arr[*count];
    (*count)++;
    strncpy(t->result, result, MAX_LEXEME - 1);
    t->result[MAX_LEXEME - 1] = '\0';
    strncpy(t->arg1, arg1, MAX_LEXEME - 1);
    t->arg1[MAX_LEXEME - 1] = '\0';
    t->has_op = 0;
    t->has_arg2 = 0;
    if (op) {
        strncpy(t->op, op, 7);
        t->op[7] = '\0';
        t->has_op = 1;
    }
    if (arg2) {
        strncpy(t->arg2, arg2, MAX_LEXEME - 1);
        t->arg2[MAX_LEXEME - 1] = '\0';
        t->has_arg2 = 1;
    }
}

static void emit(const char* result, const char* arg1, const char* op, const char* arg2) {
    if (fold_mode == 0) {
        emit_to(sem->tac_before, &sem->tac_before_count, result, arg1, op, arg2);
    } else {
        emit_to(sem->tac, &sem->tac_count, result, arg1, op, arg2);
    }
}

static char place_buf[MAX_LEXEME];

static const char* gen(ASTNode* node) {
    if (!node) return "0";

    if (node->type == NODE_NUMBER) {
        return node->value;
    }

    if (node->type == NODE_IDENTIFIER) {
        Symbol* s = lookup(node->value);
        if (!s) {
            char msg[160];
            snprintf(msg, sizeof(msg), "Line %d, Col %d: Undeclared variable '%s'",
                     node->line, node->column, node->value);
            add_sem_error(msg);
            declare(node->value);
        } else {
            mark_used(node->value);
        }
        return node->value;
    }

    if (node->type == NODE_UNARY) {
        const char* operand = gen(node->children[0]);
        if (fold_mode == 1 && is_number_str(operand)) {
            double v = -atof(operand);
            snprintf(place_buf, sizeof(place_buf), "%g", v);
            return place_buf;
        }
        char* temp = new_temp();
        char temp_copy[MAX_LEXEME];
        strncpy(temp_copy, temp, MAX_LEXEME - 1);
        temp_copy[MAX_LEXEME - 1] = '\0';
        emit(temp_copy, operand, "-", NULL);
        strncpy(place_buf, temp_copy, MAX_LEXEME - 1);
        return place_buf;
    }

    if (node->type == NODE_BINARY) {
        const char* left = gen(node->children[0]);
        char left_copy[MAX_LEXEME];
        strncpy(left_copy, left, MAX_LEXEME - 1);
        left_copy[MAX_LEXEME - 1] = '\0';

        const char* right = gen(node->children[1]);
        char right_copy[MAX_LEXEME];
        strncpy(right_copy, right, MAX_LEXEME - 1);
        right_copy[MAX_LEXEME - 1] = '\0';

        /* Constant Folding only in fold_mode == 1 */
        if (fold_mode == 1 && is_number_str(left_copy) && is_number_str(right_copy)) {
            double l = atof(left_copy);
            double r = atof(right_copy);
            double res = 0;
            if (strcmp(node->value, "+") == 0) res = l + r;
            else if (strcmp(node->value, "-") == 0) res = l - r;
            else if (strcmp(node->value, "*") == 0) res = l * r;
            else if (strcmp(node->value, "/") == 0) {
                if (r == 0.0) {
                    add_sem_error("Division by zero detected during constant folding");
                    res = 0;
                } else {
                    res = l / r;
                }
            }
            snprintf(place_buf, sizeof(place_buf), "%g", res);
            return place_buf;
        }

        char* temp = new_temp();
        char temp_copy[MAX_LEXEME];
        strncpy(temp_copy, temp, MAX_LEXEME - 1);
        temp_copy[MAX_LEXEME - 1] = '\0';
        emit(temp_copy, left_copy, node->value, right_copy);
        strncpy(place_buf, temp_copy, MAX_LEXEME - 1);
        return place_buf;
    }

    if (node->type == NODE_ASSIGN) {
        const char* value = gen(node->children[0]);
        declare(node->value);
        Symbol* s = lookup(node->value);
        if (s && is_number_str(value)) {
            s->value = atof(value);
            s->has_value = 1;
        }
        emit(node->value, value, NULL, NULL);
        return node->value;
    }

    if (node->type == NODE_DECLARATION) {
        declare(node->value);
        if (node->child_count > 0) {
            const char* value = gen(node->children[0]);
            Symbol* s = lookup(node->value);
            if (s && is_number_str(value)) {
                s->value = atof(value);
                s->has_value = 1;
            }
            emit(node->value, value, NULL, NULL);
        }
        return node->value;
    }

    if (node->type == NODE_RETURN) {
        const char* val = gen(node->children[0]);
        emit("return", val, NULL, NULL);
        return "return";
    }

    if (node->type == NODE_PARAM) {
        declare(node->value);
        return node->value;
    }

    if (node->type == NODE_FUNCTION) {
        // Just process children (params and body)
        for (int i = 0; i < node->child_count; i++) {
            gen(node->children[i]);
        }
        return node->value;
    }

    if (node->type == NODE_PARAM_LIST || node->type == NODE_PROGRAM) {
        for (int i = 0; i < node->child_count; i++) {
            gen(node->children[i]);
        }
        return "";
    }

    return "0";
}

SemanticResult analyze_and_generate(ASTNode* root) {
    SemanticResult res;
    memset(&res, 0, sizeof(res));
    sem = &res;

    if (!root) return res;

    /* Pass 1: Generate TAC WITHOUT constant folding (Before) */
    fold_mode = 0;
    sem->temp_count = 0;
    gen(root);

    /* Reset symbol table side-effects that we want clean for pass 2
       (we keep symbols, just regenerate TAC) */
    int saved_sym_count = sem->symbol_count;
    Symbol saved_syms[MAX_SYMBOLS];
    memcpy(saved_syms, sem->symbols, sizeof(Symbol) * saved_sym_count);

    /* Pass 2: Generate TAC WITH constant folding (After) */
    fold_mode = 1;
    sem->temp_count = 0;
    sem->symbol_count = 0;          /* rebuild symbols cleanly */
    sem->error_count = 0;           /* errors collected in pass 2 */
    memset(sem->symbols, 0, sizeof(sem->symbols));
    gen(root);

    return res;
}
