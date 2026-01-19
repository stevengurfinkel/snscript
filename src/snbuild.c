#include <stdlib.h>
#include <assert.h>
#include "snscript_internal.h"

sn_value_t sn_null = { .type = SN_VALUE_TYPE_NULL };

sn_error_t sn_expr_set_rtype(sn_expr_t *expr);

sn_error_t sn_symbol_set_rtype(sn_expr_t *expr)
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

    return SN_SUCCESS;
}

sn_error_t sn_let_expr_check(sn_expr_t *expr)
{
    if (expr->child_count != 3) {
        return sn_expr_error(expr, SN_ERROR_LET_EXPR_NOT_3_ITEMS);
    }

    if (expr->child_head->next->rtype != SN_RTYPE_VAR) {
        return sn_expr_error(expr, SN_ERROR_LET_EXPR_BAD_DEST);
    }

    return SN_SUCCESS;
}

sn_error_t sn_fn_expr_check(sn_expr_t *expr)
{
    if (expr->child_count < 3) {
        return sn_expr_error(expr, SN_ERROR_FN_EXPR_TOO_SHORT);
    }

    sn_expr_t *proto = expr->child_head->next;
    if (proto->rtype != SN_RTYPE_CALL) {
        return sn_expr_error(expr, SN_ERROR_FN_PROTO_NOT_LIST);
    }

    for (sn_expr_t *var = proto->child_head; var != NULL; var = var->next) {
        if (var->rtype != SN_RTYPE_VAR) {
            return sn_expr_error(expr, SN_ERROR_FN_PROTO_COTAINS_NON_SYMBOLS);
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_if_expr_check(sn_expr_t *expr)
{
    if (expr->child_count != 3 && expr->child_count != 4) {
        return sn_expr_error(expr, SN_ERROR_IF_EXPR_INVALID_LENGTH);
    }

    return SN_SUCCESS;
}

sn_error_t sn_list_set_rtype(sn_expr_t *expr)
{
    sn_error_t status = SN_SUCCESS;

    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        status = sn_expr_set_rtype(child);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    if (expr->rtype == SN_RTYPE_PROGRAM) {
        return SN_SUCCESS;
    }

    if (expr->child_count == 0) {
        return sn_expr_error(expr, SN_ERROR_EMPTY_EXPR);
    }

    switch (expr->child_head->rtype) {
        case SN_RTYPE_PROGRAM:
        case SN_RTYPE_INVALID:
            abort();
            break;
        case SN_RTYPE_LET_KEYW:
            expr->rtype = SN_RTYPE_LET_EXPR;
            status = sn_let_expr_check(expr);
            if (status != SN_SUCCESS) {
                return status;
            }
            break;
        case SN_RTYPE_FN_KEYW:
            expr->rtype = SN_RTYPE_FN_EXPR;
            status = sn_fn_expr_check(expr);
            if (status != SN_SUCCESS) {
                return status;
            }
            break;
        case SN_RTYPE_IF_KEYW:
            expr->rtype = SN_RTYPE_IF_EXPR;
            status = sn_if_expr_check(expr);
            if (status != SN_SUCCESS) {
                return status;
            }
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

    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        if (child->rtype == SN_RTYPE_FN_EXPR) {
            return sn_expr_error(child, SN_ERROR_NESTED_FN_EXPR);
        }
        else if (child->rtype == SN_RTYPE_LET_EXPR && expr->rtype != SN_RTYPE_FN_EXPR) {
            return sn_expr_error(child, SN_ERROR_NESTED_LET_EXPR);
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_set_rtype(sn_expr_t *expr)
{
    switch (expr->type) {
        case SN_EXPR_TYPE_INVALID:
            abort();
            return SN_ERROR_GENERIC;
        case SN_EXPR_TYPE_INTEGER:
            expr->rtype = SN_RTYPE_LITERAL;
            return SN_SUCCESS;
        case SN_EXPR_TYPE_SYMBOL:
            return sn_symbol_set_rtype(expr);
        case SN_EXPR_TYPE_LIST:
            return sn_list_set_rtype(expr);
    }
    return SN_ERROR_GENERIC;
}

bool sn_program_lookup_symbol(sn_program_t *prog, sn_symbol_t *sym, sn_ref_t *ref_out)
{
    ref_out->scope = SN_SCOPE_GLOBAL;
    ref_out->index = sn_symvec_idx(&prog->global_idxs, sym);
    return ref_out->index >= 0;
}

sn_error_t sn_expr_link_vars(sn_expr_t *expr, sn_rtype_t parent_type)
{
    sn_program_t *prog = expr->prog;

    if (expr->rtype == SN_RTYPE_LET_EXPR) {
        assert(expr->child_head->rtype == SN_RTYPE_LET_KEYW);

        sn_expr_t *name = expr->child_head->next;

        name->ref.scope = SN_SCOPE_GLOBAL;
        name->ref.index = sn_symvec_append(&prog->global_idxs, name->sym);
        if (name->ref.index < 0) {
            return sn_expr_error(name, SN_ERROR_REDECLARED);
        }
    }

    if (expr->rtype == SN_RTYPE_VAR) {
        if (!sn_program_lookup_symbol(prog, expr->sym, &expr->ref)) {
            return sn_expr_error(expr, SN_ERROR_UNDECLARED);
        }
    }

    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        sn_error_t status = sn_expr_link_vars(child, expr->rtype);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_program_build(sn_program_t *prog)
{
    sn_error_t status = sn_expr_set_rtype(&prog->expr);
    if (status != SN_SUCCESS) {
        return status;
    }
    return sn_expr_link_vars(&prog->expr, SN_RTYPE_INVALID);
}
