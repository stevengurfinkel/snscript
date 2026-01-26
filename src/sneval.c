#include <assert.h>
#include <stdlib.h>
#include "snscript_internal.h"

sn_error_t sn_program_run(sn_program_t *prog, sn_value_t *value_out)
{
    *value_out = sn_null;
    sn_error_t status = sn_program_build(prog);
    if (status != SN_SUCCESS) {
        return status;
    }

    prog->global_values = alloca(prog->globals.idxs.count * sizeof prog->global_values[0]);

    sn_builtin_value_t *bvalue = prog->builtin_head;
    for (int i = 0; i < prog->builtin_count; i++) {
        prog->global_values[prog->builtin_count - i - 1] = bvalue->value;
        bvalue = bvalue->next;
    }
    assert(bvalue == NULL);

    for (int i = prog->builtin_count; i < prog->globals.idxs.count; i++) {
        prog->global_values[i] = sn_null;
    }

    for (sn_expr_t *expr = prog->expr.child_head; expr != NULL; expr = expr->next) {
        status = sn_expr_eval(expr, value_out);
        if (status != SN_SUCCESS) {
            return status;
        }
    }

    return status;
}

void sn_program_lookup_ref(sn_program_t *prog, sn_ref_t *ref, sn_value_t *val_out)
{
    assert(ref->scope == SN_SCOPE_TYPE_GLOBAL);
    *val_out = prog->global_values[ref->index];
}

sn_error_t sn_expr_eval_call(sn_expr_t *expr, sn_value_t *val_out)
{
    sn_error_t status = SN_SUCCESS;
    sn_value_t *values = alloca(expr->child_count * sizeof values[0]);
    sn_expr_t *child = expr->child_head;

    for (int i = 0; i < expr->child_count; i++) {
        status = sn_expr_eval(child, &values[i]);
        if (status != SN_SUCCESS) {
            return status;
        }
        child = child->next;
    }

    if (values[0].type != SN_VALUE_TYPE_BUILTIN_FN) {
        return SN_ERROR_CALLEE_NOT_A_FN;
    }

    return values->builtin_fn(val_out, expr->child_count - 1, values + 1);
}

sn_error_t sn_expr_eval_let(sn_expr_t *expr, sn_value_t *val_out)
{
    sn_program_t *prog = expr->prog;
    sn_expr_t *kw = expr->child_head;
    sn_expr_t *var = kw->next;
    sn_expr_t *value = var->next;

    assert(kw->rtype == SN_RTYPE_LET_KEYW);
    assert(var->rtype == SN_RTYPE_VAR);
    assert(var->ref.scope == SN_SCOPE_TYPE_GLOBAL);

    *val_out = sn_null;
    return sn_expr_eval(value, &prog->global_values[var->ref.index]);
}

void sn_expr_eval_literal(sn_expr_t *expr, sn_value_t *val_out)
{
    val_out->type = SN_VALUE_TYPE_INTEGER;
    val_out->i = expr->vint;
}

sn_error_t sn_expr_eval(sn_expr_t *expr, sn_value_t *val_out)
{
    switch (expr->rtype) {
        case SN_RTYPE_LITERAL:
            sn_expr_eval_literal(expr, val_out);
            return SN_SUCCESS;

        case SN_RTYPE_VAR:
            sn_program_lookup_ref(expr->prog, &expr->ref, val_out);
            return SN_SUCCESS;

        case SN_RTYPE_CALL:
            return sn_expr_eval_call(expr, val_out);

        case SN_RTYPE_LET_EXPR:
            return sn_expr_eval_let(expr, val_out);

        case SN_EXPR_TYPE_INVALID:
        default:
            break;
    }

    abort();
    return SN_ERROR_GENERIC;
}
