#include <stdlib.h>
#include <assert.h>
#include "snscript_internal.h"

sn_value_t sn_null = { .type = SN_VALUE_TYPE_NULL };

sn_error_t sn_expr_set_rtype(sn_expr_t *expr);
sn_error_t sn_expr_link_vars(sn_expr_t *expr, sn_scope_t *scope);

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

sn_scope_type_t sn_scope_type(sn_scope_t *scope)
{
    return scope->parent == NULL ? SN_SCOPE_TYPE_GLOBAL : SN_SCOPE_TYPE_LOCAL;
}

sn_error_t sn_scope_add_var(sn_scope_t *scope, sn_expr_t *expr)
{
    sn_error_t status = sn_scope_find_var(scope, expr->sym, NULL);
    if (status == SN_SUCCESS) {
        return SN_ERROR_REDECLARED;
    }

    expr->ref.type = sn_scope_type(scope);
    expr->ref.index = scope->decl_count;

    expr->next_decl = scope->decl_head;
    scope->decl_head = expr;
    scope->decl_count++;

    return SN_SUCCESS;
}

sn_value_t *sn_scope_create_const(sn_scope_t *scope, const sn_ref_t *ref)
{
    sn_const_t *c = calloc(1, sizeof *c);
    c->idx = ref->index;
    c->next = scope->head_const;
    scope->head_const = c;

    return &c->value;
}

void sn_scope_init_consts(sn_scope_t *scope, sn_value_t *values)
{
    for (sn_const_t *c = scope->head_const; c != NULL; c = c->next) {
        values[c->idx] = c->value;
    }
}

sn_error_t sn_scope_find_var(sn_scope_t *scope, sn_symbol_t *name, sn_ref_t *ref)
{
    if (scope == NULL) {
        return SN_ERROR_UNDECLARED;
    }

    for (sn_expr_t *decl = scope->decl_head; decl != NULL; decl = decl->next_decl) {
        if (name == decl->sym) {
            if (ref != NULL) {
                *ref = decl->ref;
            }
            return SN_SUCCESS;
        }
    }

    return sn_scope_find_var(scope->parent, name, ref);
}

sn_error_t sn_expr_create_fn(sn_expr_t *expr, sn_scope_t *parent_scope)
{
    sn_func_t *func = calloc(sizeof *func, 1);
    assert(expr->child_head->rtype == SN_RTYPE_FN_KEYW);

    sn_expr_t *proto = expr->child_head->next;
    assert(proto->rtype == SN_RTYPE_CALL);
    sn_expr_t *name = proto->child_head;

    sn_error_t status = sn_scope_add_var(parent_scope, name);
    if (status != SN_SUCCESS) {
        return sn_expr_error(name, status);
    }

    sn_value_t *val = sn_scope_create_const(parent_scope, &name->ref);
    val->type = SN_VALUE_TYPE_USER_FN;
    val->user_fn = func;

    sn_scope_t scope = { .parent = parent_scope };

    for (sn_expr_t *param = proto->child_head->next; param != NULL; param = param->next) {
        status = sn_scope_add_var(&scope, param);
        if (status != SN_SUCCESS) {
            return sn_expr_error(param, status);
        }
        assert(param->ref.index == func->param_count);
        func->param_count++;
    }

    func->body = proto->next;

    for (sn_expr_t *expr = func->body; expr != NULL; expr = expr->next) {
        status = sn_expr_link_vars(expr, &scope);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_link_vars(sn_expr_t *expr, sn_scope_t *scope)
{
    if (expr->rtype == SN_RTYPE_LET_EXPR) {
        assert(expr->child_head->rtype == SN_RTYPE_LET_KEYW);

        sn_expr_t *name = expr->child_head->next;

        sn_error_t status = sn_expr_link_vars(name->next, scope);
        if (status != SN_SUCCESS) {
            return status;
        }

        status = sn_scope_add_var(scope, name);
        if (status != SN_SUCCESS) {
            return sn_expr_error(name, status);
        }
    }
    else if (expr->rtype == SN_RTYPE_FN_EXPR) {
        sn_error_t status = sn_expr_create_fn(expr, scope);
        if (status != SN_SUCCESS) {
            return status;
        }
    }
    else if (expr->rtype == SN_RTYPE_VAR) {
        sn_error_t status = sn_scope_find_var(scope, expr->sym, &expr->ref);
        if (status != SN_SUCCESS) {
            return sn_expr_error(expr, status);
        }
    }

    if (expr->rtype != SN_RTYPE_LET_EXPR && expr->rtype != SN_RTYPE_FN_EXPR) {
        for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
            sn_error_t status = sn_expr_link_vars(child, scope);
            if (status != SN_SUCCESS) {
                return status;
            }
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
    return sn_expr_link_vars(&prog->expr, &prog->globals);
}
