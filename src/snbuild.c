#include <stdlib.h>
#include <assert.h>
#include "snscript_internal.h"

sn_value_t sn_null = { .type = SN_VALUE_TYPE_NULL };
sn_value_t sn_false = { .type = SN_VALUE_TYPE_BOOLEAN, .i = false };
sn_value_t sn_true = { .type = SN_VALUE_TYPE_BOOLEAN, .i = true };

sn_error_t sn_expr_set_rtype(sn_expr_t *expr);
sn_error_t sn_expr_build(sn_expr_t *expr, sn_scope_t *scope);

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
    else if (sym == prog->sn_do) {
        expr->rtype = SN_RTYPE_DO_KEYW;
    }
    else if (sym == prog->sn_assign) {
        expr->rtype = SN_RTYPE_ASSIGN_KEYW;
    }
    else if (sym == prog->sn_const) {
        expr->rtype = SN_RTYPE_CONST_KEYW;
    }
    else if (sym == prog->sn_and) {
        expr->rtype = SN_RTYPE_AND_KEYW;
    }
    else if (sym == prog->sn_or) {
        expr->rtype = SN_RTYPE_OR_KEYW;
    }
    else if (sym == prog->sn_while) {
        expr->rtype = SN_RTYPE_WHILE_KEYW;
    }
    else if (sym == prog->sn_pure) {
        expr->rtype = SN_RTYPE_PURE_KEYW;
    }
    else {
        expr->rtype = SN_RTYPE_VAR;
    }

    return SN_SUCCESS;
}

sn_error_t sn_let_expr_check(sn_expr_t *expr)
{
    if (expr->child_count != 3) {
        return sn_expr_error(expr, SN_ERROR_EXPR_NOT_3_ITEMS);
    }

    sn_expr_t *dst = expr->child_head->next;
    if (dst->rtype != SN_RTYPE_VAR) {
        return sn_expr_error(dst, SN_ERROR_EXPR_BAD_DEST);
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

sn_error_t sn_do_expr_check(sn_expr_t *expr)
{
    if (expr->child_count < 2) {
        return sn_expr_error(expr, SN_ERROR_DO_EXPR_TOO_SHORT);
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

sn_error_t sn_lazy_expr_check(sn_expr_t *expr)
{
    if (expr->child_count < 3) {
        return sn_expr_error(expr, SN_ERROR_LAZY_EXPR_TOO_SHORT);
    }

    return SN_SUCCESS;
}

sn_error_t sn_while_expr_check(sn_expr_t *expr)
{
    if (expr->child_count < 2 || expr->child_count > 3) {
        return sn_expr_error(expr, SN_ERROR_WHILE_EXPR_WRONG_LENGTH);
    }

    return SN_SUCCESS;
}


bool sn_rtype_is_decl(sn_rtype_t type)
{
    return type == SN_RTYPE_LET_EXPR || type == SN_RTYPE_CONST_EXPR;
}

bool sn_rtype_allows_decl(sn_rtype_t type)
{
    return type == SN_RTYPE_FN_EXPR || type == SN_RTYPE_DO_EXPR;
}

sn_error_t sn_list_set_rtype_from_first_child_rtype(sn_expr_t *expr, sn_rtype_t rtype)
{
    switch (rtype) {
        case SN_RTYPE_PROGRAM:
        case SN_RTYPE_INVALID:
            abort();
            break;

        case SN_RTYPE_LET_KEYW:
            expr->rtype = SN_RTYPE_LET_EXPR;
            return sn_let_expr_check(expr);

        case SN_RTYPE_FN_KEYW:
            expr->rtype = SN_RTYPE_FN_EXPR;
            return sn_fn_expr_check(expr);

        case SN_RTYPE_IF_KEYW:
            expr->rtype = SN_RTYPE_IF_EXPR;
            return sn_if_expr_check(expr);

        case SN_RTYPE_DO_KEYW:
            expr->rtype = SN_RTYPE_DO_EXPR;
            return sn_do_expr_check(expr);

        case SN_RTYPE_ASSIGN_KEYW:
            expr->rtype = SN_RTYPE_ASSIGN_EXPR;
            return sn_let_expr_check(expr);

        case SN_RTYPE_CONST_KEYW:
            expr->rtype = SN_RTYPE_CONST_EXPR;
            return sn_let_expr_check(expr);

        case SN_RTYPE_AND_KEYW:
            expr->rtype = SN_RTYPE_AND_EXPR;
            return sn_lazy_expr_check(expr);

        case SN_RTYPE_OR_KEYW:
            expr->rtype = SN_RTYPE_OR_EXPR;
            return sn_lazy_expr_check(expr);

        case SN_RTYPE_WHILE_KEYW:
            expr->rtype = SN_RTYPE_WHILE_EXPR;
            return sn_while_expr_check(expr);

        case SN_RTYPE_PURE_KEYW:
            expr->rtype = SN_RTYPE_PURE_EXPR;
            return sn_fn_expr_check(expr);

        case SN_RTYPE_LET_EXPR:
        case SN_RTYPE_FN_EXPR:
        case SN_RTYPE_IF_EXPR:
        case SN_RTYPE_DO_EXPR:
        case SN_RTYPE_ASSIGN_EXPR:
        case SN_RTYPE_CONST_EXPR:
        case SN_RTYPE_AND_EXPR:
        case SN_RTYPE_OR_EXPR:
        case SN_RTYPE_WHILE_EXPR:
        case SN_RTYPE_PURE_EXPR:
        case SN_RTYPE_VAR:
        case SN_RTYPE_LITERAL:
        case SN_RTYPE_CALL:
            expr->rtype = SN_RTYPE_CALL;
            return SN_SUCCESS;
    }

    return SN_ERROR_GENERIC;
}

bool sn_rtype_only_in_fn(sn_rtype_t rtype)
{
    return rtype != SN_RTYPE_CONST_EXPR &&
           rtype != SN_RTYPE_LET_EXPR &&
           rtype != SN_RTYPE_FN_EXPR &&
           rtype != SN_RTYPE_PURE_EXPR;
}

sn_error_t sn_list_set_rtype(sn_expr_t *expr)
{
    sn_error_t status = SN_SUCCESS;

    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        status = sn_expr_set_rtype(child);
        if (status != SN_SUCCESS) {
            return status;
        }

        if (expr->rtype == SN_RTYPE_PROGRAM && sn_rtype_only_in_fn(child->rtype)) {
            return sn_expr_error(child, SN_ERROR_EXPR_OUTSIDE_OF_FN);
        }
    }

    if (expr->rtype == SN_RTYPE_PROGRAM) {
        return SN_SUCCESS;
    }

    if (expr->child_count == 0) {
        return sn_expr_error(expr, SN_ERROR_EMPTY_EXPR);
    }

    status = sn_list_set_rtype_from_first_child_rtype(expr, expr->child_head->rtype);
    if (status != SN_SUCCESS) {
        return status;
    }

    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        if (child->rtype == SN_RTYPE_FN_EXPR) {
            return sn_expr_error(child, SN_ERROR_NESTED_FN_EXPR);
        }
        else if (sn_rtype_is_decl(child->rtype) && !sn_rtype_allows_decl(expr->rtype)) {
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

sn_error_t sn_expr_check_fn_call(sn_expr_t *fn_expr, sn_scope_t *scope)
{
    if (!scope->is_pure) {
        return SN_SUCCESS;
    }

    // function calls must be a direct variable
    if (fn_expr->rtype != SN_RTYPE_VAR) {
        return sn_expr_error(fn_expr, SN_ERROR_NOT_ALLOWED_IN_PURE_FN);
    }

    // functions are global
    sn_ref_t *ref = &fn_expr->ref;
    if (ref->type != SN_SCOPE_TYPE_GLOBAL) {
        return sn_expr_error(fn_expr, SN_ERROR_NOT_ALLOWED_IN_PURE_FN);
    }

    // lookup in globals
    sn_value_t *val = sn_scope_get_const_value(scope->parent, ref);
    assert(val != NULL);

    if (val->type == SN_VALUE_TYPE_BUILTIN_FN) {
        if (val->builtin_fn->is_pure) {
            return SN_SUCCESS;
        }
        return sn_expr_error(fn_expr, SN_ERROR_NOT_ALLOWED_IN_PURE_FN);
    }
    else if (val->type == SN_VALUE_TYPE_USER_FN) {
        if (val->user_fn->is_pure) {
            return SN_SUCCESS;
        }
        return sn_expr_error(fn_expr, SN_ERROR_NOT_ALLOWED_IN_PURE_FN);
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_build_children(sn_expr_t *expr, sn_scope_t *scope)
{
    for (sn_expr_t *child = expr->child_head; child != NULL; child = child->next) {
        sn_error_t status = sn_expr_build(child, scope);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return sn_expr_check_fn_call(expr->child_head, scope);
}

sn_error_t sn_expr_create_fn(sn_expr_t *expr, sn_scope_t *parent_scope)
{
    sn_func_t *func = calloc(sizeof *func, 1);
    assert(expr->child_head->rtype == SN_RTYPE_FN_KEYW ||
           expr->child_head->rtype == SN_RTYPE_PURE_KEYW);

    func->is_pure = expr->child_head->rtype == SN_RTYPE_PURE_KEYW;

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

    func->scope.parent = parent_scope;
    func->scope.is_pure = func->is_pure;

    // go through all of the parameters
    for (sn_expr_t *param = proto->child_head->next; param != NULL; param = param->next) {
        status = sn_scope_add_var(&func->scope, param);
        if (status != SN_SUCCESS) {
            return sn_expr_error(param, status);
        }
        func->param_count++;
    }
    assert(func->scope.cur_decl_count == func->param_count);

    // if this is the main function, do some extra stuff
    sn_program_t *prog = expr->prog;
    if (name->sym == prog->sn_main && name->ref.type == SN_SCOPE_TYPE_GLOBAL) {
        prog->main_ref = name->ref;
        if (func->param_count > 1) {
            return sn_expr_error(proto, SN_ERROR_TOO_MANY_PARAMS_FOR_MAIN_FN);
        }
    }

    func->body = proto->next;
    func->body_count = expr->child_count - 2;

    for (sn_expr_t *expr = func->body; expr != NULL; expr = expr->next) {
        status = sn_expr_build(expr, &func->scope);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_build_decl(sn_expr_t *expr, sn_scope_t *scope, sn_expr_t **name_out)
{
    sn_expr_t *keyw = expr->child_head;
    assert(keyw->rtype == SN_RTYPE_LET_KEYW || keyw->rtype == SN_RTYPE_CONST_KEYW);

    sn_expr_t *name = keyw->next;
    if (name_out != NULL) {
        *name_out = name;
    }

    sn_error_t status = sn_expr_build(name->next, scope);
    if (status != SN_SUCCESS) {
        return status;
    }

    status = sn_scope_add_var(scope, name);
    if (status != SN_SUCCESS) {
        return sn_expr_error(name, status);
    }

    if (name->ref.type == SN_SCOPE_TYPE_GLOBAL && name->sym == expr->prog->sn_main) {
        return sn_expr_error(name, SN_ERROR_GLOBAL_MAIN_NOT_FN);
    }

    return status;
}

sn_error_t sn_expr_build_let(sn_expr_t *expr, sn_scope_t *scope)
{
    return sn_expr_build_decl(expr, scope, NULL);
}

sn_error_t sn_expr_build_const(sn_expr_t *expr, sn_scope_t *scope)
{
    sn_expr_t *name = NULL;
    sn_error_t status = sn_expr_build_decl(expr, scope, &name);
    if (status != SN_SUCCESS) {
        return status;
    }

    name->ref.is_const = true;
    return SN_SUCCESS;
}

sn_error_t sn_expr_build_var(sn_expr_t *expr, sn_scope_t *scope)
{
    sn_error_t status = sn_scope_find_var(scope, expr->sym, &expr->ref);
    if (status != SN_SUCCESS) {
        return sn_expr_error(expr, status);
    }

    if (scope->is_pure && expr->ref.type == SN_SCOPE_TYPE_GLOBAL && !expr->ref.is_const) {
        return sn_expr_error(expr, SN_ERROR_NOT_ALLOWED_IN_PURE_FN);
    }

    return status;
}

sn_error_t sn_expr_build_do(sn_expr_t *expr, sn_scope_t *scope)
{
    sn_block_t block = {0};
    sn_block_enter(&block, scope);

    sn_error_t status = sn_expr_build_children(expr, scope);
    if (status != SN_SUCCESS) {
        return status;
    }

    sn_block_leave(&block);
    return SN_SUCCESS;
}

sn_error_t sn_expr_build_assign(sn_expr_t *expr, sn_scope_t *scope)
{
    sn_expr_t *dst = expr->child_head->next;
    sn_expr_t *src = dst->next;
    assert(dst->rtype == SN_RTYPE_VAR);

    sn_error_t status = sn_expr_build_var(dst, scope);
    if (status != SN_SUCCESS) {
        return status;
    }

    if (dst->ref.is_const) {
        return sn_expr_error(dst, SN_ERROR_EXPR_BAD_DEST);
    }

    return sn_expr_build(src, scope);
}

sn_error_t sn_expr_build(sn_expr_t *expr, sn_scope_t *scope)
{
    switch (expr->rtype) {
        case SN_RTYPE_INVALID:
            abort();
            return SN_ERROR_GENERIC;

        // just return success for terminals
        case SN_RTYPE_LET_KEYW:
        case SN_RTYPE_FN_KEYW:
        case SN_RTYPE_IF_KEYW:
        case SN_RTYPE_DO_KEYW:
        case SN_RTYPE_ASSIGN_KEYW:
        case SN_RTYPE_CONST_KEYW:
        case SN_RTYPE_AND_KEYW:
        case SN_RTYPE_OR_KEYW:
        case SN_RTYPE_WHILE_KEYW:
        case SN_RTYPE_PURE_KEYW:
        case SN_RTYPE_LITERAL:
            return SN_SUCCESS;

        case SN_RTYPE_LET_EXPR:
            return sn_expr_build_let(expr, scope);

        case SN_RTYPE_FN_EXPR:
        case SN_RTYPE_PURE_EXPR:
            return sn_expr_create_fn(expr, scope);

        case SN_RTYPE_IF_EXPR:
            return sn_expr_build_children(expr, scope);

        case SN_RTYPE_DO_EXPR:
            return sn_expr_build_do(expr, scope);

        case SN_RTYPE_ASSIGN_EXPR:
            return sn_expr_build_assign(expr, scope);

        case SN_RTYPE_CONST_EXPR:
            return sn_expr_build_const(expr, scope);

        case SN_RTYPE_VAR:
            return sn_expr_build_var(expr, scope);

        case SN_RTYPE_CALL:
        case SN_RTYPE_PROGRAM:
        case SN_RTYPE_AND_EXPR:
        case SN_RTYPE_OR_EXPR:
        case SN_RTYPE_WHILE_EXPR:
            return sn_expr_build_children(expr, scope);
    }

    return SN_ERROR_GENERIC;
}

sn_error_t sn_program_build(sn_program_t *prog)
{
    sn_error_t status = sn_expr_set_rtype(&prog->expr);
    if (status != SN_SUCCESS) {
        return status;
    }

    status = sn_expr_build(&prog->expr, &prog->globals);
    if (status != SN_SUCCESS) {
        return status;
    }

    if (prog->main_ref.type == SN_SCOPE_TYPE_INVALID) {
        return SN_ERROR_MAIN_FN_MISSING;
    }

    return SN_SUCCESS;
}
