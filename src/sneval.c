#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "snscript_internal.h"

sn_env_t *sn_env_create(sn_scope_t *scope, sn_env_t *parent)
{
    sn_env_t *env = NULL;
    size_t bytes = sizeof *env + scope->max_decl_count * sizeof env->backing[0];
    env = calloc(bytes, 1);

    if (parent == NULL) {
        env->globals = env->backing;
    }
    else {
        env->globals = parent->globals;
        env->locals = env->backing;
    }

    sn_scope_init_consts(scope, env->backing);
    return env;
}

void sn_env_destroy(sn_env_t *env)
{
    free(env);
}

sn_value_t *sn_env_lookup_ref(sn_env_t *env, sn_ref_t *ref)
{
    assert(ref->type == SN_SCOPE_TYPE_GLOBAL || ref->type == SN_SCOPE_TYPE_LOCAL);

    if (ref->type == SN_SCOPE_TYPE_GLOBAL) {
        return &env->globals[ref->index];
    }
    return &env->locals[ref->index];
}

sn_error_t sn_program_run_main(sn_program_t *prog, sn_value_t *arg, sn_value_t *value_out)
{
    *value_out = sn_null;
    sn_error_t status = SN_SUCCESS;
    sn_env_t *env = sn_env_create(&prog->globals, NULL);

    for (sn_expr_t *expr = prog->expr.child_head; expr != NULL; expr = expr->next) {
        status = sn_expr_eval(expr, env, value_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    if (prog->main_ref.type == SN_SCOPE_TYPE_INVALID) {
        return SN_ERROR_MAIN_FN_MISSING;
    }

    sn_value_t *main_val = sn_env_lookup_ref(env, &prog->main_ref);
    assert(main_val->type == SN_VALUE_TYPE_USER_FN);
    sn_func_t *func = main_val->user_fn;

    sn_env_t *call_env = sn_env_create(&func->scope, env);

    if (func->param_count == 1) {
        if (arg != NULL) {
            call_env->locals[0] = *arg;
        }
        else {
            call_env->locals[0] = sn_null;
        }
    }

    for (sn_expr_t *expr = func->body; expr != NULL; expr = expr->next) {
        status = sn_expr_eval(expr, call_env, value_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    sn_env_destroy(call_env);
    sn_env_destroy(env);

    return SN_SUCCESS;
}

sn_error_t sn_call_eval_args(sn_expr_t *arg, int arg_count, sn_env_t *env, sn_value_t *args)
{
    for (int i = 0; i < arg_count; i++) {
        sn_error_t status = sn_expr_eval(arg, env, &args[i]);
        if (status != SN_SUCCESS) {
            return status;
        }
        arg = arg->next;
    }

    assert(arg == NULL);
    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_call(sn_eval_t *e)
{
    sn_value_t fn_value = {0};
    sn_expr_t *child = e->expr->child_head;
    sn_error_t status = sn_expr_eval(child, e->env, &fn_value);
    if (status != SN_SUCCESS) {
        return status;
    }

    int arg_count = e->expr->child_count - 1;

    if (fn_value.type == SN_VALUE_TYPE_BUILTIN_FN) {
        size_t args_size = sizeof (sn_value_t) * arg_count;
        sn_value_t *args = alloca(args_size);
        memset(args, '\0', args_size);

        status = sn_call_eval_args(child->next, arg_count, e->env, args);
        if (status != SN_SUCCESS) {
            return status;
        }

        status = fn_value.builtin_fn(e->val_out, arg_count, args);
        if (status != SN_SUCCESS) {
            return sn_expr_error(e->expr, status);
        }
    }
    else if (fn_value.type == SN_VALUE_TYPE_USER_FN) {
        sn_func_t *func = fn_value.user_fn;
        if (arg_count != func->param_count) {
            return sn_expr_error(e->expr, SN_ERROR_WRONG_ARG_COUNT_IN_CALL);
        }

        sn_env_t *call_env = sn_env_create(&func->scope, e->env);

        status = sn_call_eval_args(child->next, arg_count, e->env, call_env->locals);
        if (status != SN_SUCCESS) {
            return status;
        }

        for (sn_expr_t *expr = func->body; expr != NULL; expr = expr->next) {
            status = sn_expr_eval(expr, call_env, e->val_out);
            if (status != SN_SUCCESS) {
                return status;
            }
        }

        sn_env_destroy(call_env);
    }
    else {
        abort();
        return SN_ERROR_GENERIC;
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_decl(sn_eval_t *e)
{
    sn_expr_t *kw = e->expr->child_head;
    sn_expr_t *var = kw->next;
    sn_expr_t *value = var->next;

    assert(kw->rtype == SN_RTYPE_LET_KEYW || kw->rtype == SN_RTYPE_CONST_KEYW);
    assert(var->rtype == SN_RTYPE_VAR);

    *e->val_out = sn_null;
    return sn_expr_eval(value, e->env, sn_env_lookup_ref(e->env, &var->ref));
}

void sn_expr_eval_literal(sn_eval_t *e)
{
    e->val_out->type = SN_VALUE_TYPE_INTEGER;
    e->val_out->i = e->expr->vint;
}

sn_error_t sn_expr_eval_if(sn_eval_t *e)
{
    sn_value_t cond = {0};

    sn_expr_t *check = e->expr->child_head->next;
    sn_expr_t *true_arm = check->next;
    sn_expr_t *false_arm = true_arm->next; // maybe NULL

    sn_error_t status = sn_expr_eval(check, e->env, &cond);
    if (status != SN_SUCCESS) {
        return status;
    }

    if (cond.type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(check, SN_ERROR_WRONG_VALUE_TYPE);
    }

    // evaluate true arm
    if (cond.i) {
        return sn_expr_eval(true_arm, e->env, e->val_out);
    }

    // false arm present
    if (false_arm != NULL) {
        return sn_expr_eval(false_arm, e->env, e->val_out);
    }

    // false arm absent
    *e->val_out = sn_null;
    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_do(sn_eval_t *e)
{
    for (sn_expr_t *child = e->expr->child_head->next; child != NULL; child = child->next) {
        sn_error_t status = sn_expr_eval(child, e->env, e->val_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_assign(sn_eval_t *e)
{
    *e->val_out = sn_null;
    sn_expr_t *dst = e->expr->child_head->next;
    sn_expr_t *src = dst->next;

    return sn_expr_eval(src, e->env, sn_env_lookup_ref(e->env, &dst->ref));
}

sn_error_t sn_expr_eval_and(sn_eval_t *e)
{
    for (sn_expr_t *child = e->expr->child_head->next; child != NULL; child = child->next) {
        sn_error_t status = sn_expr_eval(child, e->env, e->val_out);
        if (status != SN_SUCCESS) {
            return status;
        }

        if (e->val_out->type != SN_VALUE_TYPE_BOOLEAN) {
            return sn_expr_error(child, SN_ERROR_WRONG_VALUE_TYPE);
        }

        // short-circuit
        if (e->val_out->i == false) {
            return SN_SUCCESS;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_or(sn_eval_t *e)
{
    for (sn_expr_t *child = e->expr->child_head->next; child != NULL; child = child->next) {
        sn_error_t status = sn_expr_eval(child, e->env, e->val_out);
        if (status != SN_SUCCESS) {
            return status;
        }

        if (e->val_out->type != SN_VALUE_TYPE_BOOLEAN) {
            return sn_expr_error(child, SN_ERROR_WRONG_VALUE_TYPE);
        }

        // short-circuit
        if (e->val_out->i == true) {
            return SN_SUCCESS;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_eval_while(sn_eval_t *e)
{
    sn_expr_t *cond_expr = e->expr->child_head->next;
    sn_expr_t *body_start = cond_expr->next; // maybe null

    *e->val_out = sn_null;

    sn_value_t cond = { .type = SN_VALUE_TYPE_INVALID };
    while (true) {
        sn_error_t status = sn_expr_eval(cond_expr, e->env, &cond);
        if (status != SN_SUCCESS) {
            return status;
        }

        if (cond.type != SN_VALUE_TYPE_BOOLEAN) {
            return sn_expr_error(cond_expr, SN_ERROR_WRONG_VALUE_TYPE);
        }

        if (!cond.i) {
            break;
        }

        for (sn_expr_t *body = body_start; body != NULL; body = body->next) {
            sn_error_t status = sn_expr_eval(body, e->env, e->val_out);
            if (status != SN_SUCCESS) {
                return status;
            }
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_expr_eval(sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    sn_eval_t eval = { .expr = expr, .env = env, .val_out = val_out, .parent = NULL };
    switch (expr->rtype) {
        case SN_RTYPE_LITERAL:
            sn_expr_eval_literal(&eval);
            return SN_SUCCESS;

        case SN_RTYPE_VAR:
            *val_out =  *sn_env_lookup_ref(env, &expr->ref);
            return SN_SUCCESS;

        case SN_RTYPE_CALL:
            return sn_expr_eval_call(&eval);

        case SN_RTYPE_LET_EXPR:
        case SN_RTYPE_CONST_EXPR:
            return sn_expr_eval_decl(&eval);

        case SN_RTYPE_FN_EXPR:
            *val_out = sn_null;
            return SN_SUCCESS;

        case SN_RTYPE_IF_EXPR:
            return sn_expr_eval_if(&eval);

        case SN_RTYPE_DO_EXPR:
            return sn_expr_eval_do(&eval);

        case SN_RTYPE_ASSIGN_EXPR:
            return sn_expr_eval_assign(&eval);

        case SN_RTYPE_AND_EXPR:
            return sn_expr_eval_and(&eval);

        case SN_RTYPE_OR_EXPR:
            return sn_expr_eval_or(&eval);

        case SN_RTYPE_WHILE_EXPR:
            return sn_expr_eval_while(&eval);

        case SN_EXPR_TYPE_INVALID:
        default:
            break;
    }

    abort();
    return SN_ERROR_GENERIC;
}
