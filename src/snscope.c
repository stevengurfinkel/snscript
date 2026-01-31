#include <stdlib.h>
#include "snscript_internal.h"

sn_scope_type_t sn_scope_type(sn_scope_t *scope)
{
    return scope->parent == NULL ? SN_SCOPE_TYPE_GLOBAL : SN_SCOPE_TYPE_LOCAL;
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

sn_ref_t *sn_scope_find_var_current_scope(sn_scope_t *scope, sn_symbol_t *name)
{
    for (sn_expr_t *decl = scope->decl_head; decl != NULL; decl = decl->next_decl) {
        if (name == decl->sym) {
            return &decl->ref;
        }
    }

    return NULL;
}

sn_error_t sn_scope_add_var(sn_scope_t *scope, sn_expr_t *expr)
{
    if (sn_scope_find_var_current_scope(scope, expr->sym) != NULL) {
        return SN_ERROR_REDECLARED;
    }

    expr->ref.type = sn_scope_type(scope);
    expr->ref.index = scope->decl_count;

    expr->next_decl = scope->decl_head;
    scope->decl_head = expr;
    scope->decl_count++;

    return SN_SUCCESS;
}

sn_error_t sn_scope_find_var(sn_scope_t *scope, sn_symbol_t *name, sn_ref_t *ref)
{
    for (sn_scope_t *cur = scope; cur != NULL; cur = cur->parent) {
        sn_ref_t *found = sn_scope_find_var_current_scope(cur, name);
        if (found != NULL) {
            *ref = *found;
            return SN_SUCCESS;
        }
    }

    return SN_ERROR_UNDECLARED;
}
