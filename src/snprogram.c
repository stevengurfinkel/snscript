#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include "snprogram.h"

void sn_symbol_next(sn_symbol_t *pos)
{
    assert(pos->length > 0);
    pos->value++;
    pos->length--;
}

void sn_symbol_skip_whitespace(sn_symbol_t *pos)
{
    while (pos->length > 0 && isspace(pos->value[0])) {
        sn_symbol_next(pos);
    }
}

int64_t sn_symbol_parse_integer(sn_symbol_t *pos)
{
    int64_t sign = 1;
    int64_t value = 0;

    if (pos->value[0] == '-') {
        sign = -1;
        sn_symbol_next(pos);
    }

    while (pos->length > 0 && isdigit(pos->value[0])) {
        value = 10 * value + pos->value[0] - '0';
        sn_symbol_next(pos);
    }

    return sign * value;
}

bool sn_symbol_is_integer(sn_symbol_t *pos)
{
    char c = pos->value[0];
    return isdigit(c) || (c == '-' && pos->length >= 2 && isdigit(pos->value[1]));
}

sn_sexpr_t *sn_program_parse_sexpr(sn_program_t *prog, sn_symbol_t *pos)
{
    sn_symbol_skip_whitespace(pos);
    if (pos->length == 0) {
        return NULL;
    }

    sn_sexpr_t *expr = calloc(1, sizeof *expr);

    if (sn_symbol_is_integer(pos)) {
        expr->type = SN_SEXPR_TYPE_INTEGER;
        expr->vint = sn_symbol_parse_integer(pos);
    }
    return expr;
}

sn_program_t *sn_program_create(const char *source, size_t size)
{
    sn_program_t *prog = calloc(1, sizeof *prog);
    prog->src.value = malloc(size);
    prog->src.length = size;
    memcpy(prog->src.value, source, size);

    prog->expr_tail = &prog->expr_head;
    prog->symbol_tail = &prog->symbol_head;

    sn_sexpr_t *expr = NULL;
    sn_symbol_t src = prog->src;
    while ((expr = sn_program_parse_sexpr(prog, &src)) != NULL) {
        *prog->expr_tail = expr;
        prog->expr_tail = &expr->next;
    }
    
    return prog;
}

void sn_program_destroy(sn_program_t *prog)
{
    if (prog == NULL) {
        return;
    }

    free(prog->src.value);
    free(prog);
}

sn_sexpr_t *sn_program_test_get_first_sexpr(sn_program_t *prog)
{
    return prog->expr_head;
}

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str)
{
    size_t len = strlen(str);
    if (len != sym->length) {
        return false;
    }

    return memcmp(sym->value, str, sym->length) == 0;
}
