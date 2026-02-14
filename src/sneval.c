#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "snscript_internal.h"

#define SN_STACK_SIZE 1024

sn_stack_t *sn_stack_create(int frame_count)
{
    sn_stack_t *stack = NULL;
    size_t size = sizeof stack[0] + frame_count * sizeof stack[0].frames[0];
    stack = calloc(1, size);
    stack->frame_count = frame_count;
    stack->frame_idx = -1;
    return stack;
};

void sn_stack_destroy(sn_stack_t *stack)
{
    free(stack);
}

sn_error_t
sn_stack_init_top(sn_stack_t *stack, sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    sn_frame_t *f = &stack->frames[stack->frame_idx];
    memset(f, '\0', sizeof *f);
    f->stack = stack;
    f->expr = expr;
    f->env = env;
    f->val_out = val_out;
    *val_out = sn_null;
    return SN_SUCCESS;
}

sn_error_t
sn_frame_push_noadvance(sn_frame_t *f,
                        sn_expr_t *expr,
                        sn_env_t *env,
                        sn_value_t *val_out)
{
    sn_stack_t *stack = f->stack;
    if (stack->frame_idx == stack->frame_count) {
        return sn_expr_error(expr, SN_ERROR_GENERIC);
    }

    stack->frame_idx++;
    return sn_stack_init_top(stack, expr, env, val_out);
}

sn_error_t sn_frame_goto(sn_frame_t *f, int pos, sn_expr_t *expr)
{
    f->cont_pos = pos;
    f->cont_child = expr;
    return SN_SUCCESS;
}

sn_error_t
sn_frame_push_goto(sn_frame_t *f, int pos, sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    f->cont_pos = pos;
    if (expr != NULL) {
        return sn_frame_push_noadvance(f, expr, env, val_out);
    }
    return SN_SUCCESS;
}

sn_error_t sn_frame_push_pos(sn_frame_t *f, sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    f->cont_pos++;
    return sn_frame_push_noadvance(f, expr, env, val_out);
}

sn_error_t sn_frame_push_expr(sn_frame_t *f, sn_env_t *env, sn_value_t *val_out)
{
    sn_expr_t *expr = f->cont_child;
    f->cont_child = expr->next;
    return sn_frame_push_noadvance(f, expr, env, val_out);
}

sn_error_t sn_frame_emplace(sn_frame_t *f, sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out)
{
    sn_stack_t *stack = f->stack;
    assert(f == &stack->frames[stack->frame_idx]);
    return sn_stack_init_top(stack, expr, env, val_out);
}

sn_error_t sn_frame_pop(sn_frame_t *f)
{
    sn_stack_t *stack = f->stack;
    assert(stack->frame_idx >= 0);
    stack->frame_idx--;
    return SN_SUCCESS;
}

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

sn_error_t sn_frame_eval_call(sn_frame_t *f)
{
    sn_expr_t *fn_expr = f->expr->child_head;
    sn_call_frame_t *call = &f->call;
    int arg_count = f->expr->child_count - 1;
    sn_error_t status = SN_SUCCESS;

    switch (f->cont_pos) {
        case SN_CALL_FRAME_POS_EVAL_FN:
            return sn_frame_push_pos(f, fn_expr, f->env, &call->fn);

        case SN_CALL_FRAME_POS_ALLOC_ENV:
            if (call->fn.type == SN_VALUE_TYPE_USER_FN) {
                sn_func_t *func = call->fn.user_fn;
                if (arg_count != func->param_count) {
                    return sn_expr_error(f->expr, SN_ERROR_WRONG_ARG_COUNT_IN_CALL);
                }

                call->env = sn_env_create(&func->scope, f->env);
                call->args = call->env->locals;
            }
            else if (call->fn.type == SN_VALUE_TYPE_BUILTIN_FN) {
                call->args = calloc(arg_count, sizeof call->args[0]);
            }
            else {
                return sn_expr_error(fn_expr, SN_ERROR_CALLEE_NOT_A_FN);
            }

            return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_ARGS, fn_expr->next);

        case SN_CALL_FRAME_POS_EVAL_ARGS:
            if (f->cont_child != NULL) {
                return sn_frame_push_expr(f, f->env, &call->args[call->arg_idx++]);
            }

            if (call->fn.type == SN_VALUE_TYPE_BUILTIN_FN) {
                return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_BUILTIN, NULL);
            }

            return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_USER, call->fn.user_fn->body);

        case SN_CALL_FRAME_POS_EVAL_BUILTIN:
            status = call->fn.builtin_fn(f->val_out, arg_count, call->args);
            free(call->args);
            if (status != SN_SUCCESS) {
                return sn_expr_error(f->expr, status);
            }
            return sn_frame_pop(f);

        case SN_CALL_FRAME_POS_EVAL_USER:
            if (f->cont_child != NULL) {
                return sn_frame_push_expr(f, call->env, f->val_out);
            }
            sn_env_destroy(call->env);
            return sn_frame_pop(f);

        default:
            break;
    }

    abort();
    return SN_ERROR_GENERIC;
}

sn_error_t sn_frame_eval_assign(sn_frame_t *f)
{
    sn_expr_t *dst = f->expr->child_head->next;
    sn_expr_t *src = dst->next;

    if (f->cont_pos == 0) {
        return sn_frame_push_pos(f, src, f->env, &f->cond);
    }

    *sn_env_lookup_ref(f->env, &dst->ref) = f->cond;
    return sn_frame_pop(f);
}

sn_error_t sn_frame_eval_if(sn_frame_t *f)
{
    sn_expr_t *cond = f->expr->child_head->next;
    sn_expr_t *true_arm = cond->next;
    sn_expr_t *false_arm = true_arm->next; // maybe NULL

    if (f->cont_pos == 0) {
        return sn_frame_push_pos(f, cond, f->env, &f->cond);
    }

    if (f->cond.type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(cond, SN_ERROR_WRONG_VALUE_TYPE);
    }

    if (f->cond.i) {
        return sn_frame_emplace(f, true_arm, f->env, f->val_out);
    }

    if (false_arm != NULL) {
        return sn_frame_emplace(f, false_arm, f->env, f->val_out);
    }

    return sn_frame_pop(f);
}

sn_error_t sn_frame_eval_do(sn_frame_t *f)
{
    if (f->cont_pos == 0) {
        f->cont_child = f->expr->child_head->next;
        f->cont_pos = 1;
    }

    if (f->cont_child != NULL) {
        return sn_frame_push_expr(f, f->env, f->val_out);
    }

    return sn_frame_pop(f);
}

int sn_frame_andor_default_value(sn_frame_t *f)
{
    sn_rtype_t rtype = f->expr->rtype;
    assert(rtype == SN_RTYPE_AND_EXPR || rtype == SN_RTYPE_OR_EXPR);
    return rtype == SN_RTYPE_AND_EXPR;
}

sn_expr_t *sn_frame_prev_child(sn_frame_t *f)
{
    sn_expr_t *prev = NULL;
    for (sn_expr_t *expr = f->expr->child_head; expr != f->cont_child; expr = expr->next) {
        prev = expr;
    }
    return prev;
}

sn_error_t sn_frame_eval_andor(sn_frame_t *f)
{
    if (f->cont_pos == 0) {
        f->cont_child = f->expr->child_head->next;
        f->cont_pos = 1;
        f->val_out->type = SN_VALUE_TYPE_BOOLEAN;
        f->val_out->i = sn_frame_andor_default_value(f);
    }

    if (f->cont_child != NULL) {
        if (f->val_out->type != SN_VALUE_TYPE_BOOLEAN) {
            return sn_expr_error(sn_frame_prev_child(f), SN_ERROR_WRONG_VALUE_TYPE);
        }

        if (f->val_out->i != sn_frame_andor_default_value(f)) {
            return sn_frame_pop(f);
        }

        return sn_frame_push_expr(f, f->env, f->val_out);
    }

    if (f->val_out->type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(sn_frame_prev_child(f), SN_ERROR_WRONG_VALUE_TYPE);
    }

    return sn_frame_pop(f);
}

sn_error_t sn_frame_eval_while(sn_frame_t *f)
{
    sn_expr_t *cond_expr = f->expr->child_head->next;
    sn_expr_t *body = cond_expr->next; // maybe null

    if (f->cont_pos == 0) {
        return sn_frame_push_pos(f, cond_expr, f->env, &f->cond);
    }

    if (f->cond.type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(cond_expr, SN_ERROR_WRONG_VALUE_TYPE);
    }

    if (!f->cond.i) {
        return sn_frame_pop(f);
    }

    return sn_frame_push_goto(f, 0, body, f->env, f->val_out);
}

sn_error_t sn_frame_dispatch(sn_frame_t *f)
{
    switch (f->expr->rtype) {
        case SN_RTYPE_LITERAL:
            f->val_out->type = SN_VALUE_TYPE_INTEGER;
            f->val_out->i = f->expr->vint;
            return sn_frame_pop(f);

        case SN_RTYPE_VAR:
            *f->val_out = *sn_env_lookup_ref(f->env, &f->expr->ref);
            return sn_frame_pop(f);

        case SN_RTYPE_CALL:
            return sn_frame_eval_call(f);

        case SN_RTYPE_LET_EXPR:
        case SN_RTYPE_CONST_EXPR:
        case SN_RTYPE_ASSIGN_EXPR:
            return sn_frame_eval_assign(f);

        case SN_RTYPE_FN_EXPR:
            return sn_frame_pop(f);

        case SN_RTYPE_IF_EXPR:
            return sn_frame_eval_if(f);

        case SN_RTYPE_DO_EXPR:
            return sn_frame_eval_do(f);

        case SN_RTYPE_AND_EXPR:
        case SN_RTYPE_OR_EXPR:
            return sn_frame_eval_andor(f);

        case SN_RTYPE_WHILE_EXPR:
            return sn_frame_eval_while(f);

        case SN_EXPR_TYPE_INVALID:
        default:
            break;

    }
    abort();
    return SN_ERROR_GENERIC;
}

sn_error_t
sn_eval_expr_with_stack(sn_expr_t *expr,
                        sn_stack_t *stack,
                        sn_env_t *env,
                        sn_value_t *val_out)
{
    stack->frame_idx++;
    sn_stack_init_top(stack, expr, env, val_out);

    while (stack->frame_idx >= 0) {
        sn_error_t status = sn_frame_dispatch(&stack->frames[stack->frame_idx]);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t
sn_eval_expr_list_with_stack(sn_expr_t *expr_head,
                             sn_stack_t *stack,
                             sn_env_t *env,
                             sn_value_t *val_out)
{
    for (sn_expr_t *expr = expr_head; expr != NULL; expr = expr->next) {
        sn_error_t status = sn_eval_expr_with_stack(expr, stack, env, val_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t sn_program_run_main(sn_program_t *prog, sn_value_t *arg, sn_value_t *value_out)
{
    *value_out = sn_null;
    sn_error_t status = SN_SUCCESS;
    sn_env_t *env = sn_env_create(&prog->globals, NULL);
    sn_stack_t *stack = sn_stack_create(SN_STACK_SIZE);

    status = sn_eval_expr_list_with_stack(prog->expr.child_head, stack, env, value_out);
    if (status != SN_SUCCESS) {
        return status;
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

    status = sn_eval_expr_list_with_stack(func->body, stack, call_env, value_out);
    if (status != SN_SUCCESS) {
        return status;
    }

    sn_stack_destroy(stack);
    sn_env_destroy(call_env);
    sn_env_destroy(env);

    return SN_SUCCESS;
}
