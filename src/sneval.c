#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "snscript_internal.h"

sn_error_t sn_program_run(sn_program_t *prog, sn_value_t *value_out)
{
    *value_out = sn_null;
    sn_error_t status = SN_SUCCESS;
    sn_env_t env = {0};
    size_t bytes = prog->globals.decl_count * sizeof env.globals[0];
    env.globals = alloca(bytes);
    memset(env.globals, '\0', bytes);

    sn_scope_init_consts(&prog->globals, env.globals);

    for (sn_expr_t *expr = prog->expr.child_head; expr != NULL; expr = expr->next) {
        status = sn_expr_eval(expr, &env, value_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return status;
}

sn_value_t *sn_env_lookup_ref(sn_env_t *env, sn_ref_t *ref)
{
    assert(ref->type == SN_SCOPE_TYPE_GLOBAL || ref->type == SN_SCOPE_TYPE_LOCAL);

    if (ref->type == SN_SCOPE_TYPE_GLOBAL) {
        return &env->globals[ref->index];
    }
    return &env->locals[ref->index];
}

sn_error_t sn_expr_eval_call(sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    sn_value_t fn_value = {0};
    sn_expr_t *child = expr->child_head;
    sn_error_t status = sn_expr_eval(child, env, &fn_value);
    if (status != SN_SUCCESS) {
        return status;
    }

    int arg_count = expr->child_count - 1;

    if (fn_value.type == SN_VALUE_TYPE_BUILTIN_FN) {
        size_t args_size = sizeof (sn_value_t) * arg_count;
        sn_value_t *args = alloca(args_size);
        memset(args, '\0', args_size);

        for (int i = 0; i < arg_count; i++) {
            child = child->next;
            status = sn_expr_eval(child, env, &args[i]);
            if (status != SN_SUCCESS) {
                return status;
            }
        }

        return fn_value.builtin_fn(val_out, arg_count, args);
    }
    else if (fn_value.type == SN_VALUE_TYPE_USER_FN) {
        sn_func_t *func = fn_value.user_fn;
        if (arg_count != func->param_count) {
            return sn_expr_error(expr, SN_ERROR_WRONG_ARG_COUNT_IN_CALL);
        }
    }

    return SN_ERROR_GENERIC;
}

sn_error_t sn_expr_eval_let(sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    sn_expr_t *kw = expr->child_head;
    sn_expr_t *var = kw->next;
    sn_expr_t *value = var->next;

    assert(kw->rtype == SN_RTYPE_LET_KEYW);
    assert(var->rtype == SN_RTYPE_VAR);
    assert(var->ref.type == SN_SCOPE_TYPE_GLOBAL);

    *val_out = sn_null;
    return sn_expr_eval(value, env, sn_env_lookup_ref(env, &var->ref));
}

void sn_expr_eval_literal(sn_expr_t *expr, sn_value_t *val_out)
{
    val_out->type = SN_VALUE_TYPE_INTEGER;
    val_out->i = expr->vint;
}

sn_error_t sn_expr_eval(sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    switch (expr->rtype) {
        case SN_RTYPE_LITERAL:
            sn_expr_eval_literal(expr, val_out);
            return SN_SUCCESS;

        case SN_RTYPE_VAR:
            *val_out =  *sn_env_lookup_ref(env, &expr->ref);
            return SN_SUCCESS;

        case SN_RTYPE_CALL:
            return sn_expr_eval_call(expr, env, val_out);

        case SN_RTYPE_LET_EXPR:
            return sn_expr_eval_let(expr, env, val_out);

        case SN_RTYPE_FN_EXPR:
            *val_out = sn_null;
            return SN_SUCCESS;

        case SN_EXPR_TYPE_INVALID:
        default:
            break;
    }

    abort();
    return SN_ERROR_GENERIC;
}
