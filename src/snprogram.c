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

bool sn_cur_two_more(sn_program_t *prog)
{
    return prog->cur + 1 < prog->last;
}

bool sn_cur_did_skip_comment(sn_program_t *prog)
{
    if (sn_cur_two_more(prog) && prog->cur[0] == ';' && prog->cur[1] == ';') {
        while (sn_cur_more(prog) && *prog->cur != '\n') {
            prog->cur++;
        }
        return true;
    }

    return false;
}

bool sn_cur_did_skip_whitespace(sn_program_t *prog)
{
    bool did_skip = false;
    while (sn_cur_more(prog) && isspace(*prog->cur)) {
        prog->cur++;
        did_skip = true;
    }

    return did_skip;
}

void sn_cur_skip_whitespace(sn_program_t *prog)
{
    while (sn_cur_did_skip_whitespace(prog) || sn_cur_did_skip_comment(prog));
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
    return isdigit(c) || (c == '-' && sn_cur_two_more(prog) && isdigit(prog->cur[1]));
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
        expr->child_count++;
    }
}

void sn_program_reorder_infix_expr(sn_program_t *prog, sn_sexpr_t *expr)
{
    if (expr->child_count != 3) {
        fprintf(prog->msg,
                "Error: {} expression needs 3 elments, but has %zd\n",
                expr->child_count);
        return;
    }

    sn_sexpr_t *exprs[3] = {expr->child_head,
                            expr->child_head->next,
                            expr->child_head->next->next};

    expr->child_head = exprs[1];
    expr->child_head->next = exprs[0];
    expr->child_head->next->next = exprs[2];
    expr->child_tail = NULL;
}

bool sn_cur_is_expr_end(sn_program_t *prog)
{
    return !sn_cur_more(prog) ||
           *prog->cur == ')' ||
           *prog->cur == '}';
}

sn_sexpr_t *sn_cur_parse_sexpr(sn_program_t *prog)
{
    sn_cur_skip_whitespace(prog);
    if (sn_cur_is_expr_end(prog)) {
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
    else if (*prog->cur == '{') {
        sn_cur_consume(prog, '{');
        sn_cur_parse_sexpr_list(prog, expr);
        sn_cur_consume(prog, '}');
        sn_program_reorder_infix_expr(prog, expr);
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
