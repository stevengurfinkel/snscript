#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include "snprogram.h"

sn_sexpr_t *sn_cur_parse_sexpr(sn_program_t *prog);

bool sn_cur_more(sn_program_t *prog)
{
    return prog->cur < prog->last;
}

void sn_cur_skip_whitespace(sn_program_t *prog)
{
    while (sn_cur_more(prog) && isspace(*prog->cur)) {
        prog->cur++;
    }
}

int64_t sn_cur_parse_integer(sn_program_t *prog)
{
    int64_t sign = 1;
    int64_t value = 0;

    if (*prog->cur == '-') {
        sign = -1;
        prog->cur++;
    }

    while (sn_cur_more(prog) && isdigit(*prog->cur)) {
        value = 10 * value + *prog->cur - '0';
        prog->cur++;
    }

    return sign * value;
}

bool sn_cur_is_integer(sn_program_t *prog)
{
    char c = *prog->cur;
    return isdigit(c) || (c == '-' && prog->cur + 1 < prog->last && isdigit(prog->cur[1]));
}

void sn_cur_consume(sn_program_t *prog, char c)
{
    if (!sn_cur_more(prog)) {
        fprintf(prog->msg, "Error: expected %c but got end of input\n", c);
    }
    if (*prog->cur != c) {
        fprintf(prog->msg, "Error: expected %c but got %c\n", c, *prog->cur);
    }

    prog->cur++;
}

void sn_cur_parse_sexpr_list(sn_program_t *prog, sn_sexpr_t *expr)
{
    expr->type = SN_SEXPR_TYPE_SEXPR;
    expr->child_tail = &expr->child_head;

    sn_sexpr_t *child = NULL;
    while ((child = sn_cur_parse_sexpr(prog)) != NULL) {
        *expr->child_tail = child;
        expr->child_tail = &child->next;
    }
}

sn_sexpr_t *sn_cur_parse_sexpr(sn_program_t *prog)
{
    sn_cur_skip_whitespace(prog);
    if (!sn_cur_more(prog) || *prog->cur == ')') {
        return NULL;
    }

    sn_sexpr_t *expr = calloc(1, sizeof *expr);

    if (sn_cur_is_integer(prog)) {
        expr->type = SN_SEXPR_TYPE_INTEGER;
        expr->vint = sn_cur_parse_integer(prog);
    }
    else if (*prog->cur == '(') {
        sn_cur_consume(prog, '(');
        sn_cur_parse_sexpr_list(prog, expr);
        sn_cur_consume(prog, ')');
    }
    return expr;
}

sn_program_t *sn_program_create(const char *source, size_t size)
{
    sn_program_t *prog = calloc(1, sizeof *prog);
    prog->cur = source;
    prog->last = source + size;

    prog->msg = stderr;
    sn_cur_parse_sexpr_list(prog, &prog->expr);

    prog->cur = NULL;
    prog->last = NULL;
    return prog;
}

void sn_program_destroy(sn_program_t *prog)
{
    if (prog == NULL) {
        return;
    }

    free(prog);
}

sn_sexpr_t *sn_program_test_get_first_sexpr(sn_program_t *prog)
{
    return prog->expr.child_head;
}

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str)
{
    size_t len = strlen(str);
    if (len != sym->length) {
        return false;
    }

    return memcmp(sym->value, str, sym->length) == 0;
}
