#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "snprogram.h"

sn_expr_t *sn_cur_parse_expr(sn_program_t *prog);

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

bool sn_cur_is_symbol(sn_program_t *prog)
{
    const char *ok = "!@$%^&*-_=+[]:<>./?";
    return sn_cur_more(prog) &&
           (isalnum(*prog->cur) || strchr(ok, *prog->cur) != NULL);
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

sn_symbol_t *sn_cur_parse_symbol(sn_program_t *prog)
{
    const char *start = prog->cur;
    while (sn_cur_is_symbol(prog)) {
        prog->cur++;
    }

    return sn_program_get_symbol(prog, start, prog->cur);
}

bool sn_cur_is_integer(sn_program_t *prog)
{
    char c = *prog->cur;
    return isdigit(c) || (c == '-' && sn_cur_two_more(prog) && isdigit(prog->cur[1]));
}

void sn_cur_consume(sn_program_t *prog, char c)
{
    if (!sn_cur_more(prog)) {
        prog->status = SN_ERROR_END_OF_INPUT;
        return;
    }
    if (*prog->cur != c) {
        prog->status = SN_ERROR_EXPECTED_EXPR_CLOSE;
        return;
    }

    prog->cur++;
}

void sn_cur_parse_expr_list(sn_program_t *prog, sn_expr_t *expr)
{
    expr->type = SN_SEXPR_TYPE_SEXPR;
    sn_expr_t **child_tail = &expr->child_head;

    sn_expr_t *child = NULL;
    while (prog->status == SN_SUCCESS && (child = sn_cur_parse_expr(prog)) != NULL) {
        *child_tail = child;
        child_tail = &child->next;
        expr->child_count++;
    }
}

void sn_program_reorder_infix_expr(sn_program_t *prog, sn_expr_t *expr)
{
    if (expr->child_count != 3) {
        prog->status = SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS;
        return;
    }

    sn_expr_t *exprs[3] = {expr->child_head,
                           expr->child_head->next,
                           expr->child_head->next->next};

    expr->child_head = exprs[1];
    expr->child_head->next = exprs[0];
    expr->child_head->next->next = exprs[2];
}

bool sn_cur_is_expr_end(sn_program_t *prog)
{
    return !sn_cur_more(prog) ||
           *prog->cur == ')' ||
           *prog->cur == '}';
}

sn_expr_t *sn_cur_parse_expr(sn_program_t *prog)
{
    sn_cur_skip_whitespace(prog);
    if (sn_cur_is_expr_end(prog)) {
        return NULL;
    }

    sn_expr_t *expr = calloc(1, sizeof *expr);
    expr->prog = prog;

    if (sn_cur_is_integer(prog)) {
        expr->type = SN_SEXPR_TYPE_INTEGER;
        expr->vint = sn_cur_parse_integer(prog);
    }
    else if (*prog->cur == '(') {
        sn_cur_consume(prog, '(');
        sn_cur_parse_expr_list(prog, expr);
        sn_cur_consume(prog, ')');
    }
    else if (*prog->cur == '{') {
        sn_cur_consume(prog, '{');
        sn_cur_parse_expr_list(prog, expr);
        sn_cur_consume(prog, '}');
        sn_program_reorder_infix_expr(prog, expr);
    }
    else {
        expr->type = SN_SEXPR_TYPE_SYMBOL;
        expr->sym = sn_cur_parse_symbol(prog);
    }
    return expr;
}

sn_expr_t *sn_program_test_get_first_expr(sn_program_t *prog)
{
    return prog->expr.child_head;
}
