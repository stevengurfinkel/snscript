#include <stdlib.h>
#include <assert.h>
#include "snprogram.h"

sn_value_t sn_null = { .type = SN_VALUE_TYPE_NULL };

void sn_sexpr_set_rtype(sn_sexpr_t *expr);

void sn_symbol_set_rtype(sn_sexpr_t *expr)
{
    sn_program_t *prog = expr->prog;
    sn_symbol_t *sym = expr->sym;

    if (sym == prog->sn_let) {
        expr->rtype = SN_RTYPE_LET_KEYW;
    }
    else if (sym == prog->sn_if) {
        expr->rtype = SN_RTYPE_IF_KEYW;
    }
    else if (sym == prog->sn_fn) {
        expr->rtype = SN_RTYPE_FN_KEYW;
    }
    else {
        expr->rtype = SN_RTYPE_VAR;
    }
}

void sn_let_expr_check(sn_sexpr_t *expr)
{
    sn_program_t *prog = expr->prog;
    if (expr->child_count != 3) {
        fprintf(prog->msg, "Error: let expression must have 3 items\n");
        abort();
    }

    if (expr->child_head->next->rtype != SN_RTYPE_VAR) {
        fprintf(prog->msg, "Error: let expression must assign to a variable name\n");
        abort();
    }
}

void sn_fn_expr_check(sn_sexpr_t *expr)
{
    sn_program_t *prog = expr->prog;
    if (expr->child_count < 3) {
        fprintf(prog->msg, "Error: fn expression must have at least 3 items\n");
        abort();
    }

    sn_sexpr_t *proto = expr->child_head->next;
    if (proto->rtype != SN_RTYPE_CALL) {
        fprintf(prog->msg, "Error: fn prototype must be a list\n");
        abort();
    }

    for (sn_sexpr_t *var = proto->child_head; var != NULL; var = var->next) {
        if (var->rtype != SN_RTYPE_VAR) {
            fprintf(prog->msg, "Error: fn prototype must contain only variable names\n");
            abort();
        }
    }
}

void sn_if_expr_check(sn_sexpr_t *expr)
{
    if (expr->child_count != 3 && expr->child_count != 4) {
        fprintf(expr->prog->msg, "Error: if statement must have 3 or 4 elements\n");
        abort();
    }
}

void sn_list_set_rtype(sn_sexpr_t *expr)
{
    sn_program_t *prog = expr->prog;

    for (sn_sexpr_t *child = expr->child_head; child != NULL; child = child->next) {
        sn_sexpr_set_rtype(child);
    }

    if (expr->rtype == SN_RTYPE_PROGRAM) {
        return;
    }

    if (expr->child_count == 0) {
        fprintf(prog->msg, "Error: empty list\n");
        abort();
    }

    switch (expr->child_head->rtype) {
        case SN_RTYPE_PROGRAM:
        case SN_RTYPE_INVALID:
            abort();
            break;
        case SN_RTYPE_LET_KEYW:
            expr->rtype = SN_RTYPE_LET_EXPR;
            sn_let_expr_check(expr);
            break;
        case SN_RTYPE_FN_KEYW:
            expr->rtype = SN_RTYPE_FN_EXPR;
            sn_fn_expr_check(expr);
            break;
        case SN_RTYPE_IF_KEYW:
            expr->rtype = SN_RTYPE_IF_EXPR;
            sn_if_expr_check(expr);
            break;
        case SN_RTYPE_LET_EXPR:
        case SN_RTYPE_FN_EXPR:
        case SN_RTYPE_IF_EXPR:
        case SN_RTYPE_VAR:
        case SN_RTYPE_LITERAL:
        case SN_RTYPE_CALL:
            expr->rtype = SN_RTYPE_CALL;
            break;
    }

    for (sn_sexpr_t *child = expr->child_head; child != NULL; child = child->next) {
        if (child->rtype == SN_RTYPE_FN_EXPR) {
            fprintf(prog->msg, "Error: functions must be defined at the top level\n");
            abort();
        }
        else if (child->rtype == SN_RTYPE_LET_EXPR && expr->rtype != SN_RTYPE_FN_EXPR) {
            fprintf(prog->msg, "Error: let cannot be nested in a non-function\n");
            abort();
        }
    }
}

void sn_sexpr_set_rtype(sn_sexpr_t *expr)
{
    switch (expr->type) {
        case SN_SEXPR_TYPE_INVALID:
            abort();
            break;
        case SN_SEXPR_TYPE_INTEGER:
            expr->rtype = SN_RTYPE_LITERAL;
            break;
        case SN_SEXPR_TYPE_SYMBOL:
            sn_symbol_set_rtype(expr);
            break;
        case SN_SEXPR_TYPE_SEXPR:
            sn_list_set_rtype(expr);
            break;
    }
}

bool sn_program_lookup_symbol(sn_program_t *prog, sn_symbol_t *sym, sn_ref_t *ref_out)
{
    ref_out->scope = SN_SCOPE_GLOBAL;
    ref_out->index = sn_symvec_idx(&prog->global_idxs, sym);
    return ref_out->index > 0;
}

void sn_sexpr_link_vars(sn_sexpr_t *expr, sn_rtype_t parent_type)
{
    sn_program_t *prog = expr->prog;

    if (expr->rtype == SN_RTYPE_LET_EXPR) {
        assert(expr->child_head->rtype == SN_RTYPE_LET_KEYW);

        sn_sexpr_t *name = expr->child_head->next;

        name->ref.scope = SN_SCOPE_GLOBAL;
        name->ref.index = sn_symvec_append(&prog->global_idxs, name->sym);
    }

    if (expr->rtype == SN_RTYPE_VAR) {
        if (!sn_program_lookup_symbol(prog, expr->sym, &expr->ref)) {
            fprintf(prog->msg, "Error: %s is undeclared\n", expr->sym->value);
            abort();
        }
    }

    for (sn_sexpr_t *child = expr->child_head; child != NULL; child = child->next) {
        sn_sexpr_link_vars(child, expr->rtype);
    }
}

void sn_program_build(sn_program_t *prog)
{
    sn_sexpr_set_rtype(&prog->expr);
    sn_sexpr_link_vars(&prog->expr, SN_RTYPE_INVALID);
}
