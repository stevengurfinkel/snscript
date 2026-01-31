#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include "snscript_internal.h"

sn_error_t sn_cur_parse_expr(sn_program_t *prog, sn_expr_t **expr_out);

sn_error_t sn_cur_error(sn_program_t *prog, sn_error_t status)
{
    prog->error_line = prog->cur_line;
    prog->error_col = prog->cur_col;
    return status;
}

bool sn_cur_more(sn_program_t *prog)
{
    return prog->cur < prog->last;
}

void sn_cur_next(sn_program_t *prog)
{
    assert(sn_cur_more(prog));
    prog->cur_col++;
    if (*prog->cur == '\n') {
        prog->cur_line++;
        prog->cur_col = 1;
    }
    prog->cur++;
}

bool sn_cur_two_more(sn_program_t *prog)
{
    return prog->cur + 1 < prog->last;
}

bool sn_cur_is_expr_end(sn_program_t *prog)
{
    return !sn_cur_more(prog) ||
           *prog->cur == ')' ||
           *prog->cur == '}';
}

bool sn_cur_is_whitespace(sn_program_t *prog)
{
    return sn_cur_more(prog) && isspace(*prog->cur);
}

bool sn_cur_is_comment_start(sn_program_t *prog)
{
    return sn_cur_two_more(prog) && prog->cur[0] == ';' && prog->cur[1] == ';';
}

bool sn_cur_is_end_of_token(sn_program_t *prog)
{
    return sn_cur_is_expr_end(prog) ||
           sn_cur_is_comment_start(prog) ||
           sn_cur_is_whitespace(prog);
}

bool sn_cur_did_skip_comment(sn_program_t *prog)
{
    if (sn_cur_is_comment_start(prog)) {
        while (sn_cur_more(prog) && *prog->cur != '\n') {
            sn_cur_next(prog);
        }
        return true;
    }

    return false;
}

bool sn_cur_did_skip_whitespace(sn_program_t *prog)
{
    bool did_skip = false;
    while (sn_cur_is_whitespace(prog)) {
        sn_cur_next(prog);
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
    const char *ok = "!@$%^&*-_=+:<>./?|";
    return sn_cur_more(prog) &&
           (isalnum(*prog->cur) || strchr(ok, *prog->cur) != NULL);
}

sn_error_t sn_cur_parse_integer(sn_program_t *prog, sn_expr_t *expr)
{
    int64_t sign = 1;
    int64_t value = 0;

    if (*prog->cur == '-') {
        sign = -1;
        sn_cur_next(prog);
    }

    while (sn_cur_more(prog) && isdigit(*prog->cur)) {
        value = 10 * value + *prog->cur - '0';
        sn_cur_next(prog);
    }

    if (!sn_cur_is_end_of_token(prog)) {
        return sn_cur_error(prog, SN_ERROR_INVALID_INTEGER_LITERAL);
    }

    expr->type = SN_EXPR_TYPE_INTEGER;
    expr->vint = sign * value;
    return SN_SUCCESS;
}

sn_error_t sn_cur_parse_symbol(sn_program_t *prog, sn_expr_t *expr)
{
    const char *start = prog->cur;
    while (sn_cur_is_symbol(prog)) {
        sn_cur_next(prog);
    }

    if (!sn_cur_is_end_of_token(prog)) {
        return sn_cur_error(prog, SN_ERROR_INVALID_SYMBOL_NAME);
    }

    expr->type = SN_EXPR_TYPE_SYMBOL;
    expr->sym = sn_program_get_symbol(prog, start, prog->cur);
    return SN_SUCCESS;
}

bool sn_cur_is_integer(sn_program_t *prog)
{
    char c = *prog->cur;
    return isdigit(c) || (c == '-' && sn_cur_two_more(prog) && isdigit(prog->cur[1]));
}

sn_error_t sn_cur_consume(sn_program_t *prog, char c)
{
    if (!sn_cur_more(prog)) {
        return sn_cur_error(prog, SN_ERROR_UNEXPECTED_END_OF_INPUT);
    }
    if (*prog->cur != c) {
        return sn_cur_error(prog, SN_ERROR_EXPECTED_EXPR_CLOSE);
    }

    sn_cur_next(prog);
    return SN_SUCCESS;
}

sn_error_t sn_cur_parse_expr_list(sn_program_t *prog, sn_expr_t *expr)
{
    sn_error_t status = SN_SUCCESS;
    expr->type = SN_EXPR_TYPE_LIST;
    sn_expr_t **child_tail = &expr->child_head;
    sn_expr_t *child = NULL;

    while ((status = sn_cur_parse_expr(prog, &child)) == SN_SUCCESS && child != NULL) {
        *child_tail = child;
        child_tail = &child->next;
        expr->child_count++;
    }

    return status;
}

sn_error_t sn_program_parse(sn_program_t *prog)
{
    prog->cur_line = 1;
    prog->cur_col = 1;

    prog->expr.prog = prog;
    prog->expr.rtype = SN_RTYPE_PROGRAM;

    sn_error_t status = sn_cur_parse_expr_list(prog, &prog->expr);
    if (status == SN_SUCCESS && prog->cur != prog->last) {
        return sn_cur_error(prog, SN_ERROR_EXTRA_CHARS_AT_END_OF_INPUT);
    }

    return status;
}

sn_error_t sn_program_reorder_infix_expr(sn_program_t *prog, sn_expr_t *expr)
{
    if (expr->child_count != 3) {
        return sn_expr_error(expr, SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS);
    }

    sn_expr_t *exprs[3] = {expr->child_head,
                           expr->child_head->next,
                           expr->child_head->next->next};

    expr->child_head = exprs[1];
    expr->child_head->next = exprs[0];
    expr->child_head->next->next = exprs[2];
    return SN_SUCCESS;
}

sn_error_t sn_cur_parse_expr(sn_program_t *prog, sn_expr_t **expr_out)
{
    sn_error_t status = SN_SUCCESS;
    *expr_out = NULL;

    sn_cur_skip_whitespace(prog);
    if (sn_cur_is_expr_end(prog)) {
        return SN_SUCCESS;
    }

    sn_expr_t *expr = calloc(1, sizeof *expr);
    expr->prog = prog;
    expr->line = prog->cur_line;
    expr->col = prog->cur_col;

    if (sn_cur_is_integer(prog)) {
        status = sn_cur_parse_integer(prog, expr);
        if (status != SN_SUCCESS) {
            goto Done;
        }
    }
    else if (*prog->cur == '(') {
        status = sn_cur_consume(prog, '(');
        if (status != SN_SUCCESS) {
            goto Done;
        }
        status = sn_cur_parse_expr_list(prog, expr);
        if (status != SN_SUCCESS) {
            goto Done;
        }
        status = sn_cur_consume(prog, ')');
        if (status != SN_SUCCESS) {
            goto Done;
        }
    }
    else if (*prog->cur == '{') {
        status = sn_cur_consume(prog, '{');
        if (status != SN_SUCCESS) {
            goto Done;
        }
        status = sn_cur_parse_expr_list(prog, expr);
        if (status != SN_SUCCESS) {
            goto Done;
        }
        status = sn_cur_consume(prog, '}');
        if (status != SN_SUCCESS) {
            goto Done;
        }
        status = sn_program_reorder_infix_expr(prog, expr);
        if (status != SN_SUCCESS) {
            goto Done;
        }
    }
    else {
        status = sn_cur_parse_symbol(prog, expr);
        if (status != SN_SUCCESS) {
            goto Done;
        }
    }

Done:
    if (status == SN_SUCCESS) {
        *expr_out = expr;
    }
    else {
        free(expr);
    }
    return status;
}

sn_expr_t *sn_program_test_get_first_expr(sn_program_t *prog)
{
    return prog->expr.child_head;
}
