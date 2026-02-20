#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "snscript_internal.h"

#define SN_STACK_FRAME_COUNT 1024
#define SN_STACK_VALUE_COUNT 65536

int sn_stack_alloc_values(sn_stack_t *stack, int count)
{
    if (stack->push_count == SN_STACK_VALUE_COUNT) {
        return -1;
    }

    int start = stack->push_count;
    stack->push_count += count;
    return start;
}

bool sn_stack_is_empty(sn_stack_t *stack)
{
    return stack->frame_top == SN_STACK_FRAME_COUNT;
}

void sn_stack_init(sn_stack_t *stack, sn_scope_t *globals)
{
    stack->frame_top = SN_STACK_FRAME_COUNT;
    stack->frames = calloc(stack->frame_top, sizeof stack->frames[0]);

    stack->push_count = 0;
    stack->values = calloc(SN_STACK_VALUE_COUNT, sizeof stack->values[0]);

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
        assert(f->base_push_count == stack->push_count);
        return &stack->values[sn_stack_alloc_values(stack, 1)];
    }

    return &stack->values[f->base_push_count];
}

sn_error_t
sn_stack_init_top(sn_stack_t *stack, sn_expr_t *expr, int locals_idx, sn_value_t *val_out)
{
    sn_frame_t *f = sn_stack_top(stack);
    memset(f, '\0', sizeof *f);
    f->expr = expr;
    f->locals_idx = locals_idx;
    f->base_push_count = stack->push_count;
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

sn_error_t sn_frame_goto(sn_frame_t *f, int pos)
{
    f->cont_pos = pos;
    return SN_SUCCESS;
}

sn_error_t sn_stack_pop(sn_stack_t *stack)
{
    assert(stack->frame_top < SN_STACK_FRAME_COUNT);
    stack->push_count = stack->frames[stack->frame_top].base_push_count;
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

sn_error_t sn_frame_check_call(sn_stack_t *stack, sn_frame_t *f, sn_value_t *fn)
{
    int arg_count = f->expr->child_count - 1;

    if (fn->type == SN_VALUE_TYPE_USER_FN) {
        sn_func_t *func = fn->user_fn;
        if (arg_count != func->param_count) {
            return sn_expr_error(f->expr, SN_ERROR_WRONG_ARG_COUNT_IN_CALL);
        }

        // TODO: check for overflow
        stack->push_count += func->scope.max_decl_count - func->param_count;
    }
    else if (fn->type != SN_VALUE_TYPE_BUILTIN_FN) {
        return sn_expr_error(f->expr->child_head, SN_ERROR_CALLEE_NOT_A_FN);
    }

    return SN_SUCCESS;
}

sn_error_t
sn_stack_eval_call_body(sn_stack_t *stack,
                        sn_frame_t *f,
                        int call_value_count,
                        sn_value_t *call_values)
{
    sn_error_t status = SN_SUCCESS;
    int body_idx = f->cont_pos++ - call_value_count;
    sn_value_t *fn = &call_values[0];

    if (fn->type == SN_VALUE_TYPE_USER_FN) {
        sn_func_t *func = fn->user_fn;
        if (body_idx < func->body_count) {
            return sn_stack_push(stack, &func->body[body_idx], f->val_out);
        }
    }
    else {
        status = fn->builtin_fn->fn(f->val_out,
                                    call_value_count - 1,
                                    call_values + 1);
        if (status != SN_SUCCESS) {
            return sn_expr_error(f->expr, status);
        }
    }

    return sn_stack_pop(stack);
}

sn_error_t sn_stack_eval_call(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_expr_t *fn_expr = f->expr->child_head;
    int call_value_count = f->expr->child_count;

    sn_value_t *call_values = &stack->values[f->base_push_count];

    if (f->cont_pos == 0) {
        // TODO: check for overflow
        stack->push_count += call_value_count;
        f->cont_pos++;
        return sn_stack_push(stack, fn_expr, call_values);
    }
    else if (f->cont_pos == 1) {
        sn_error_t status = sn_frame_check_call(stack, f, call_values);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    if (f->cont_pos > 0 && f->cont_pos < call_value_count) {
        sn_value_t *arg_value = &call_values[f->cont_pos];
        sn_expr_t *expr_value = fn_expr + f->cont_pos;
        f->cont_pos++;
        return sn_stack_push(stack, expr_value, arg_value);
    }
    else if (f->cont_pos == call_value_count) {
        f->locals_idx = f->base_push_count + 1;
    }

    return sn_stack_eval_call_body(stack, f, call_value_count, call_values);
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
        f->cont_pos = 1;
    }

    if (f->cont_pos < f->expr->child_count) {
        return sn_stack_push(stack, &f->expr->child_head[f->cont_pos++], f->val_out);
    }

    return sn_stack_pop(stack);
}

int sn_frame_andor_default_value(sn_frame_t *f)
{
    sn_rtype_t rtype = f->expr->rtype;
    assert(rtype == SN_RTYPE_AND_EXPR || rtype == SN_RTYPE_OR_EXPR);
    return rtype == SN_RTYPE_AND_EXPR;
}

sn_error_t sn_stack_eval_andor(sn_stack_t *stack)
{
    sn_frame_t *f = sn_stack_top(stack);
    sn_value_t *val = f->val_out;

    if (f->cont_pos == 0) {
        val->type = SN_VALUE_TYPE_BOOLEAN;
        val->i = sn_frame_andor_default_value(f);
        f->cont_pos = 1;
    }

    sn_expr_t *child = &f->expr->child_head[f->cont_pos];

    if (val->type != SN_VALUE_TYPE_BOOLEAN) {
        return sn_expr_error(child - 1, SN_ERROR_WRONG_VALUE_TYPE);
    }

    if (val->i != sn_frame_andor_default_value(f) || f->cont_pos == f->expr->child_count) {
        return sn_stack_pop(stack);
    }

    f->cont_pos++;
    return sn_stack_push(stack, child, val);
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

    return sn_frame_goto(f, 1);
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

        case SN_RTYPE_PURE_EXPR:
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

    assert(prog->main_ref.type == SN_SCOPE_TYPE_GLOBAL);

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
