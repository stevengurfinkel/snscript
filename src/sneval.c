#include <stdlib.h>
#include "snprogram.h"

sn_value_t sn_null = { .type = SN_VALUE_TYPE_NULL };

void sn_sexpr_set_refs(sn_sexpr_t *expr, sn_program_t *prog)
{
    if (expr->type == SN_SEXPR_TYPE_SEXPR) {
        for (sn_sexpr_t *child = expr->child_head; child != NULL; child = child->next) {
            sn_sexpr_set_refs(child, prog);
        }
    }
    else if (expr->type == SN_SEXPR_TYPE_SYMBOL) {
        expr->ref.scope = SN_SCOPE_BUILTIN;
        expr->ref.index = sn_symvec_idx(&prog->builtin_idxs, expr->sym);
        if (expr->ref.index < 0) {
            fprintf(prog->msg, "Error: %s is not declared\n", expr->sym->value);
            abort();
        }
    }
}

void sn_program_set_refs(sn_program_t *prog)
{
    sn_sexpr_set_refs(&prog->expr, prog);   
}

sn_value_t sn_program_run(sn_program_t *prog)
{
    sn_program_set_refs(prog);

    sn_value_t value = sn_null;
    for (sn_sexpr_t *expr = prog->expr.child_head; expr != NULL; expr = expr->next) {
        value = sn_program_eval_expr(prog, expr);
    }
    return value;
}

sn_value_t sn_program_lookup_ref(sn_program_t *prog, sn_ref_t *ref)
{
    if (ref->scope == SN_SCOPE_BUILTIN) {
        return prog->builtin_values[ref->index];
    }
    abort();
    return sn_null;
}

sn_value_t sn_program_eval_call(sn_program_t *prog, sn_sexpr_t *expr)
{
    if (expr->child_count == 0) {
        fprintf(prog->msg, "Error: empty function call\n");
        abort();
    }

    sn_value_t *values = alloca(expr->child_count * sizeof values[0]);
    sn_sexpr_t *child = expr->child_head;
    for (int i = 0; i < expr->child_count; i++) {
        values[i] = sn_program_eval_expr(prog, child);
        child = child->next;
    }

    if (values->type != SN_VALUE_TYPE_BUILTIN_FN) {
        fprintf(prog->msg, "Error: attempt to call something that is not a function\n");
        abort();
    }

    sn_value_t ret;
    if (!values->builtin_fn(&ret, expr->child_count - 1, values + 1)) {
        fprintf(prog->msg, "Error: builtin function returned an error\n");
        abort();
    }

    return ret;
}

sn_value_t sn_program_eval_expr(sn_program_t *prog, sn_sexpr_t *expr)
{
    switch (expr->type) {
        case SN_SEXPR_TYPE_INVALID:
            abort();
            return sn_null;
        case SN_SEXPR_TYPE_INTEGER:
            return (sn_value_t){ .type = SN_VALUE_TYPE_INTEGER, .i = expr->vint };
        case SN_SEXPR_TYPE_SYMBOL:
            return sn_program_lookup_ref(prog, &expr->ref);
        case SN_SEXPR_TYPE_SEXPR:
            return sn_program_eval_call(prog, expr);
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
