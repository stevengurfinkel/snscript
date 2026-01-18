#include <assert.h>
#include <stdlib.h>
#include "snprogram.h"

sn_error_t sn_program_run(sn_program_t *prog, sn_value_t *value_out)
{
    *value_out = sn_null;
    sn_error_t status = sn_program_build(prog);
    if (status != SN_SUCCESS) {
        return status;
    }

    prog->global_values = alloca(prog->global_idxs.count * sizeof prog->global_values[0]);

    sn_builtin_value_t *bvalue = prog->builtin_head;
    for (int i = 0; i < prog->builtin_count; i++) {
        prog->global_values[prog->builtin_count - i - 1] = bvalue->value;
        bvalue = bvalue->next;
    }
    assert(bvalue == NULL);

    for (int i = prog->builtin_count; i < prog->global_idxs.count; i++) {
        prog->global_values[i] = sn_null;
    }

    for (sn_expr_t *expr = prog->expr.child_head; expr != NULL; expr = expr->next) {
        *value_out = sn_program_eval_expr(prog, expr);
    }

    return prog->status;
}

sn_value_t sn_program_lookup_ref(sn_program_t *prog, sn_ref_t *ref)
{
    if (ref->scope == SN_SCOPE_GLOBAL) {
        return prog->global_values[ref->index];
    }
    abort();
    return sn_null;
}

sn_value_t sn_program_eval_call(sn_program_t *prog, sn_expr_t *expr)
{
    if (expr->child_count == 0) {
        fprintf(prog->msg, "Error: empty function call\n");
        abort();
    }

    sn_value_t *values = alloca(expr->child_count * sizeof values[0]);
    sn_expr_t *child = expr->child_head;
    for (int i = 0; i < expr->child_count; i++) {
        values[i] = sn_program_eval_expr(prog, child);
        child = child->next;
    }

    if (values->type != SN_VALUE_TYPE_BUILTIN_FN) {
        fprintf(prog->msg, "Error: attempt to call something that is not a function\n");
        abort();
    }

    sn_value_t ret;
    sn_error_t status = values->builtin_fn(&ret, expr->child_count - 1, values + 1);
    if (status != SN_SUCCESS) {
        fprintf(prog->msg, "Error: builtin function returned an error\n");
        abort();
    }

    return ret;
}

sn_value_t sn_expr_eval_let(sn_expr_t *expr)
{
    sn_program_t *prog = expr->prog;
    sn_expr_t *kw = expr->child_head;
    sn_expr_t *var = kw->next;
    sn_expr_t *value = var->next;

    assert(kw->rtype == SN_RTYPE_LET_KEYW);
    assert(var->rtype == SN_RTYPE_VAR);
    assert(var->ref.scope == SN_SCOPE_GLOBAL);

    prog->global_values[var->ref.index] = sn_program_eval_expr(prog, value);
    return sn_null;
}

sn_value_t sn_program_eval_expr(sn_program_t *prog, sn_expr_t *expr)
{
    switch (expr->rtype) {
        case SN_EXPR_TYPE_INVALID:
            abort();
            return sn_null;
        case SN_RTYPE_LITERAL:
            return (sn_value_t){ .type = SN_VALUE_TYPE_INTEGER, .i = expr->vint };
        case SN_RTYPE_VAR:
            return sn_program_lookup_ref(prog, &expr->ref);
        case SN_RTYPE_CALL:
            return sn_program_eval_call(prog, expr);
        case SN_RTYPE_LET_EXPR:
            return sn_expr_eval_let(expr);
        default:
            return sn_null;
    }
}

void sn_value_print(sn_value_t value, FILE *stream)
{
    switch (value.type) {
        case SN_VALUE_TYPE_INVALID:
            fprintf(stream, "!!INVALID");
            break;
        case SN_VALUE_TYPE_NULL:
            fprintf(stream, "null");
            break;
        case SN_VALUE_TYPE_INTEGER:
            fprintf(stream, "%ld", value.i);
            break;
        case SN_VALUE_TYPE_USER_FN:
            fprintf(stream, "fn(%p)", value.user_fn);
            break;
        case SN_VALUE_TYPE_BUILTIN_FN:
            fprintf(stream, "cfn(%p)", value.builtin_fn);
            break;
    }
}
