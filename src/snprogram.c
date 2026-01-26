#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include "snscript_internal.h"
#define SN_ERROR_CASE(x) case SN_ERROR_ ## x: return "SN_ERROR_" #x

const char *sn_error_str(sn_error_t status)
{
    switch (status) {
        case SN_SUCCESS: return "SN_SUCCESS";
        SN_ERROR_CASE(UNEXPECTED_END_OF_INPUT);
        SN_ERROR_CASE(EXPECTED_EXPR_CLOSE);
        SN_ERROR_CASE(INFIX_EXPR_NOT_3_ELEMENTS);
        SN_ERROR_CASE(EXTRA_CHARS_AT_END_OF_INPUT);
        SN_ERROR_CASE(INVALID_INTEGER_LITERAL);
        SN_ERROR_CASE(INVALID_SYMBOL_NAME);
        SN_ERROR_CASE(LET_EXPR_NOT_3_ITEMS);
        SN_ERROR_CASE(LET_EXPR_BAD_DEST);
        SN_ERROR_CASE(FN_EXPR_TOO_SHORT);
        SN_ERROR_CASE(FN_PROTO_NOT_LIST);
        SN_ERROR_CASE(FN_PROTO_COTAINS_NON_SYMBOLS);
        SN_ERROR_CASE(IF_EXPR_INVALID_LENGTH);
        SN_ERROR_CASE(EMPTY_EXPR);
        SN_ERROR_CASE(NESTED_FN_EXPR);
        SN_ERROR_CASE(NESTED_LET_EXPR);
        SN_ERROR_CASE(UNDECLARED);
        SN_ERROR_CASE(REDECLARED);
        SN_ERROR_CASE(CALLEE_NOT_A_FN);
        SN_ERROR_CASE(INVALID_PARAMS_TO_FN);
        SN_ERROR_CASE(WRONG_VALUE_TYPE);
        SN_ERROR_CASE(GENERIC);
    }
    return NULL;
}

sn_error_t sn_expr_error(sn_expr_t *expr, sn_error_t error)
{
    assert(error != SN_SUCCESS);

    sn_program_t *prog = expr->prog;
    prog->error_line = expr->line;
    prog->error_col = expr->col;
    if (expr->type == SN_EXPR_TYPE_SYMBOL) {
        prog->error_sym = expr->sym;
    }
    return error;
}

void sn_program_error_pos(sn_program_t *prog, int *line_out, int *col_out)
{
    *line_out = prog->error_line;
    *col_out = prog->error_col;
}

void sn_program_error_symbol(sn_program_t *prog, const char **symbol_out)
{
    *symbol_out = (prog->error_sym == NULL) ? NULL : prog->error_sym->value;
}

sn_symbol_t *sn_program_add_symbol(sn_program_t *prog, const char *str, size_t size)
{
    sn_symbol_t *sym = calloc(1, sizeof *sym + size + 1);
    sym->length = size;
    memcpy(sym->value, str, size);

    *prog->symbol_tail = sym;
    prog->symbol_tail = &sym->next;

    return sym;
}

sn_symbol_t *sn_program_default_symbol(sn_program_t *prog, const char *str)
{
    return sn_program_add_symbol(prog, str, strlen(str));
}

sn_symbol_t *sn_program_get_symbol(sn_program_t *prog, const char *start, const char *end)
{
    size_t size = end - start;
    for (sn_symbol_t *sym = prog->symbol_head; sym != NULL; sym = sym->next) {
        if (sym->length == size && memcmp(sym->value, start, size) == 0) {
            return sym;
        }
    }

    return sn_program_add_symbol(prog, start, size);
}

sn_value_t *sn_program_add_builtin_value(sn_program_t *prog, const char *str)
{
    sn_symbol_t *name = sn_program_default_symbol(prog, str);
    sn_error_t status = sn_scope_add_var(&prog->globals, name, NULL);
    assert(status == SN_SUCCESS);

    sn_builtin_value_t *bvalue = calloc(1, sizeof *bvalue);
    bvalue->next = prog->builtin_head;
    prog->builtin_head = bvalue;
    prog->builtin_count++;

    return &bvalue->value;
}

void sn_program_add_builtin_fn(sn_program_t *prog, const char *str, sn_builtin_fn_t fn)
{
    sn_value_t *value = sn_program_add_builtin_value(prog, str);
    value->type = SN_VALUE_TYPE_BUILTIN_FN;
    value->builtin_fn = fn;
}

void sn_program_add_default_symbols(sn_program_t *prog)
{
    // add keywords
    prog->sn_let = sn_program_default_symbol(prog, "let");
    prog->sn_fn = sn_program_default_symbol(prog, "fn");
    prog->sn_if = sn_program_default_symbol(prog, "if");

    // add global values
    sn_value_t *null = sn_program_add_builtin_value(prog, "null");
    null->type = SN_VALUE_TYPE_NULL;

    sn_program_add_builtin_fn(prog, "+", sn_add);
    sn_program_add_builtin_fn(prog, "-", sn_sub);
    sn_program_add_builtin_fn(prog, "println", sn_println);
}

sn_error_t sn_program_create(sn_program_t **program_out, const char *source, size_t size)
{
    sn_program_t *prog = calloc(1, sizeof *prog);
    prog->start = source;
    prog->cur = source;
    prog->last = source + size;

    prog->symbol_tail = &prog->symbol_head;
    sn_scope_init(&prog->globals, NULL);
    sn_program_add_default_symbols(prog);

    sn_error_t status = sn_program_parse(prog);

    prog->cur = NULL;
    prog->last = NULL;

    *program_out = prog;
    return status;
}

void sn_program_destroy(sn_program_t *prog)
{
    if (prog == NULL) {
        return;
    }

    free(prog);
}

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str)
{
    size_t len = strlen(str);
    if (len != sym->length) {
        return false;
    }

    return memcmp(sym->value, str, sym->length) == 0;
}
