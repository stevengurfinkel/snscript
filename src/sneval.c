#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "snscript_internal.h"

#define SN_STACK_FRAME_COUNT 1024
#define SN_STACK_VALUE_COUNT 65536

int sn_stack_alloc_values(sn_stack_t *stack, int count)
{
    if (stack->value_top - count < 0) {
        return -1;
    }

    stack->value_top -= count;
    return stack->value_top;
}

bool sn_stack_is_empty(sn_stack_t *stack)
{
    return stack->frame_top == SN_STACK_FRAME_COUNT;
}

void sn_stack_init(sn_stack_t *stack, sn_scope_t *globals)
{
    stack->frame_top = SN_STACK_FRAME_COUNT;
    stack->frames = calloc(stack->frame_top, sizeof stack->frames[0]);

    stack->value_top = SN_STACK_VALUE_COUNT;
    stack->values = calloc(stack->value_top, sizeof stack->values[0]);

    stack->globals = &stack->values[sn_stack_alloc_values(stack, globals->max_decl_count)];
    sn_scope_init_consts(globals, stack->globals);
}

void sn_stack_deinit(sn_stack_t *stack)
{
    free(stack->values);
    free(stack->frames);
}

int sn_stack_alloc_locals(sn_stack_t *stack, sn_scope_t *locals)
{
    return sn_stack_alloc_values(stack, locals->max_decl_count);
}

sn_frame_t *sn_stack_top(sn_stack_t *stack)
{
    return &stack->frames[stack->frame_top];
}

sn_value_t *sn_stack_alloc_temp(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    if (f->cont_pos == 0) {
        assert(f->base_idx == stack->value_top);
        return &stack->values[sn_stack_alloc_values(stack, 1)];
    }

    return &stack->values[f->base_idx - 1];
}

sn_error_t
sn_stack_init_top(sn_stack_t *stack, sn_expr_t *expr, int locals_idx, sn_value_t *val_out)
{
    sn_frame_t *f = sn_stack_top(stack);
    memset(f, '\0', sizeof *f);
    f->expr = expr;
    f->locals_idx = locals_idx;
    f->base_idx = stack->value_top;
    f->val_out = val_out;
    *val_out = sn_null;
    return SN_SUCCESS;
}

sn_error_t
sn_stack_push(sn_stack_t *stack, sn_expr_t *expr, sn_value_t *val_out)
{
    if (stack->frame_top == 0) {
        return sn_expr_error(expr, SN_ERROR_GENERIC);
    }

    int locals_idx = sn_stack_top(stack)->locals_idx;
    stack->frame_top--;
    return sn_stack_init_top(stack, expr, locals_idx, val_out);
}

sn_error_t sn_frame_goto(sn_frame_t *f, int pos, sn_expr_t *expr)
{
    f->cont_pos = pos;
    f->cont_child = expr;
    return SN_SUCCESS;
}

sn_expr_t *sn_frame_expr_next(sn_frame_t *f)
{
    sn_expr_t *expr = f->cont_child;
    f->cont_child = expr->next;
    return expr;
}

sn_error_t sn_stack_pop(sn_stack_t *stack)
{
    assert(stack->frame_top < SN_STACK_FRAME_COUNT);
    stack->value_top = stack->frames[stack->frame_top].base_idx;
    stack->frame_top++;
    return SN_SUCCESS;
}

sn_value_t *sn_stack_lookup_ref(sn_stack_t *stack, sn_ref_t *ref)
{
    assert(ref->type == SN_SCOPE_TYPE_GLOBAL || ref->type == SN_SCOPE_TYPE_LOCAL);

    if (ref->type == SN_SCOPE_TYPE_GLOBAL) {
        return &stack->globals[ref->index];
    }
    return &stack->values[sn_stack_top(stack)->locals_idx + ref->index];
}

sn_error_t sn_stack_eval_call(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_expr_t *fn_expr = f->expr->child_head;
    sn_call_frame_t *call = &f->call;
    int arg_count = f->expr->child_count - 1;
    sn_value_t *fn = sn_stack_alloc_temp(stack);
    sn_error_t status = SN_ERROR_GENERIC;

    switch (f->cont_pos) {
        case SN_CALL_FRAME_POS_EVAL_FN:
            f->cont_pos = SN_CALL_FRAME_POS_ALLOC_ENV;
            return sn_stack_push(stack, fn_expr, fn);

        case SN_CALL_FRAME_POS_ALLOC_ENV:
            if (fn->type == SN_VALUE_TYPE_USER_FN) {
                sn_func_t *func = fn->user_fn;
                if (arg_count != func->param_count) {
                    return sn_expr_error(f->expr, SN_ERROR_WRONG_ARG_COUNT_IN_CALL);
                }

                call->locals_idx = sn_stack_alloc_locals(stack, &func->scope);
            }
            else if (fn->type == SN_VALUE_TYPE_BUILTIN_FN) {
                call->locals_idx = sn_stack_alloc_values(stack, arg_count);
            }
            else {
                return sn_expr_error(fn_expr, SN_ERROR_CALLEE_NOT_A_FN);
            }

            return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_ARGS, fn_expr->next);

        case SN_CALL_FRAME_POS_EVAL_ARGS:
            if (f->cont_child != NULL) {
                return sn_stack_push(stack,
                                     sn_frame_expr_next(f),
                                     &stack->values[call->locals_idx + call->arg_idx++]);
            }

            f->locals_idx = call->locals_idx;

            if (fn->type == SN_VALUE_TYPE_BUILTIN_FN) {
                return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_BUILTIN, NULL);
            }

            return sn_frame_goto(f, SN_CALL_FRAME_POS_EVAL_USER, fn->user_fn->body);

        case SN_CALL_FRAME_POS_EVAL_BUILTIN:
            status = fn->builtin_fn(f->val_out, arg_count, &stack->values[f->locals_idx]);
            if (status != SN_SUCCESS) {
                return sn_expr_error(f->expr, status);
            }
            return sn_stack_pop(stack);

        case SN_CALL_FRAME_POS_EVAL_USER:
            if (f->cont_child != NULL) {
                return sn_stack_push(stack, sn_frame_expr_next(f), f->val_out);
            }
            return sn_stack_pop(stack);

        default:
            break;
    }

    abort();
    return SN_ERROR_GENERIC;
}

sn_error_t sn_stack_eval_assign(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_expr_t *dst = f->expr->child_head->next;
    sn_expr_t *src = dst->next;
    sn_value_t *value = sn_stack_alloc_temp(stack);

    if (f->cont_pos == 0) {
        f->cont_pos++;
        return sn_stack_push(stack, src, value);
    }

    *sn_stack_lookup_ref(stack, &dst->ref) = *value;
    return sn_stack_pop(stack);
}

sn_error_t sn_stack_eval_if(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_expr_t *cond_expr = f->expr->child_head->next;
    sn_expr_t *true_arm = cond_expr->next;
    sn_expr_t *false_arm = true_arm->next; // maybe NULL
    sn_value_t *cond = sn_stack_alloc_temp(stack);

    if (f->cont_pos == 0) {
        f->cont_pos++;
        return sn_stack_push(stack, cond_expr, cond);
    }
    else if (f->cont_pos == 1) {
        if (cond->type != SN_VALUE_TYPE_BOOLEAN) {
            return sn_expr_error(cond_expr, SN_ERROR_WRONG_VALUE_TYPE);
        }

        f->cont_pos++;
        if (cond->i) {
            return sn_stack_push(stack, true_arm, f->val_out);
        }

        if (false_arm != NULL) {
            return sn_stack_push(stack, false_arm, f->val_out);
        }
    }

    return sn_stack_pop(stack);
}

sn_error_t sn_stack_eval_do(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    if (f->cont_pos == 0) {
        f->cont_child = f->expr->child_head->next;
        f->cont_pos = 1;
    }

    if (f->cont_child != NULL) {
        return sn_stack_push(stack, sn_frame_expr_next(f), f->val_out);
    }

    return sn_stack_pop(stack);
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

sn_error_t sn_stack_eval_andor(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
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
            return sn_stack_pop(stack);
        }

        return sn_stack_push(stack, sn_frame_expr_next(f), f->val_out);
    }

    if (f->val_out->type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(sn_frame_prev_child(f), SN_ERROR_WRONG_VALUE_TYPE);
    }

    return sn_stack_pop(stack);
}

sn_error_t sn_stack_eval_while(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_expr_t *cond_expr = f->expr->child_head->next;
    sn_expr_t *body = cond_expr->next; // maybe null
    sn_value_t *cond = sn_stack_alloc_temp(stack);

    if (f->cont_pos == 0) {
        f->cont_pos++;
    }

    if (f->cont_pos == 1) {
        f->cont_pos++;
        return sn_stack_push(stack, cond_expr, cond);
    }

    if (cond->type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(cond_expr, SN_ERROR_WRONG_VALUE_TYPE);
    }

    if (!cond->i) {
        return sn_stack_pop(stack);
    }

    if (body != NULL) {
        f->cont_pos = 1;
        return sn_stack_push(stack, body, f->val_out);
    }

    return sn_frame_goto(f, 1, NULL);
}

sn_error_t sn_stack_dispatch(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    switch (f->expr->rtype) {
        case SN_RTYPE_LITERAL:
            f->val_out->type = SN_VALUE_TYPE_INTEGER;
            f->val_out->i = f->expr->vint;
            return sn_stack_pop(stack);

        case SN_RTYPE_VAR:
            *f->val_out = *sn_stack_lookup_ref(stack, &f->expr->ref);
            return sn_stack_pop(stack);

        case SN_RTYPE_CALL:
            return sn_stack_eval_call(stack);

        case SN_RTYPE_LET_EXPR:
        case SN_RTYPE_CONST_EXPR:
        case SN_RTYPE_ASSIGN_EXPR:
            return sn_stack_eval_assign(stack);

        case SN_RTYPE_FN_EXPR:
            return sn_stack_pop(stack);

        case SN_RTYPE_IF_EXPR:
            return sn_stack_eval_if(stack);

        case SN_RTYPE_DO_EXPR:
            return sn_stack_eval_do(stack);

        case SN_RTYPE_AND_EXPR:
        case SN_RTYPE_OR_EXPR:
            return sn_stack_eval_andor(stack);

        case SN_RTYPE_WHILE_EXPR:
            return sn_stack_eval_while(stack);

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
                        int locals_idx,
                        sn_value_t *val_out)
{
    stack->frame_top--;
    sn_stack_init_top(stack, expr, locals_idx, val_out);

    while (!sn_stack_is_empty(stack)) {
        sn_error_t status = sn_stack_dispatch(stack);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return SN_SUCCESS;
}

sn_error_t
sn_eval_expr_list_with_stack(sn_expr_t *expr_head,
                             sn_stack_t *stack,
                             int locals_idx,
                             sn_value_t *val_out)
{
    for (sn_expr_t *expr = expr_head; expr != NULL; expr = expr->next) {
        sn_error_t status = sn_eval_expr_with_stack(expr, stack, locals_idx, val_out);
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
    sn_stack_t stack = {0};
    sn_stack_init(&stack, &prog->globals);

    status = sn_eval_expr_list_with_stack(prog->expr.child_head, &stack, -1, value_out);
    if (status != SN_SUCCESS) {
        return status;
    }

    if (prog->main_ref.type == SN_SCOPE_TYPE_INVALID) {
        return SN_ERROR_MAIN_FN_MISSING;
    }

    sn_value_t *main_val = sn_stack_lookup_ref(&stack, &prog->main_ref);
    assert(main_val->type == SN_VALUE_TYPE_USER_FN);
    sn_func_t *func = main_val->user_fn;

    int locals_idx = sn_stack_alloc_locals(&stack, &func->scope);
    if (func->param_count == 1) {
        sn_value_t *locals = &stack.values[locals_idx];
        if (arg != NULL) {
            locals[0] = *arg;
        }
        else {
            locals[0] = sn_null;
        }
    }

    status = sn_eval_expr_list_with_stack(func->body, &stack, locals_idx, value_out);
    if (status != SN_SUCCESS) {
        return status;
    }

    sn_stack_deinit(&stack);
    return SN_SUCCESS;
}
