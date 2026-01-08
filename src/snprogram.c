#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#include "snprogram.h"

sn_symbol_t *sn_program_add_symbol(sn_program_t *prog, const char *str, size_t size)
{
    sn_symbol_t *sym = calloc(1, sizeof *sym + size);
    sym->length = size;
    memcpy(sym->value, str, size);

    *prog->symbol_tail = sym;
    prog->symbol_tail = &sym->next;

    return sym;
}

sn_symbol_t *sn_program_default_symbol(sn_program_t *prog, const char *str)
{
    return sn_program_add_symbol(prog, str, strlen(str));
}

sn_symbol_t *sn_program_get_symbol(sn_program_t *prog, const char *start, const char *end)
{
    size_t size = end - start;
    for (sn_symbol_t *sym = prog->symbol_head; sym != NULL; sym = sym->next) {
        if (sym->length == size && memcmp(sym->value, start, size) == 0) {
            return sym;
        }
    }

    return sn_program_add_symbol(prog, start, size);
}

sn_value_t *sn_program_add_builtin_value(sn_program_t *prog, const char *str)
{
    sn_symbol_t *name = sn_program_default_symbol(prog, str);
    int idx = sn_symvec_append(&prog->builtin_idxs, name);
    assert(idx >= 0 && idx < SN_PROGRAM_MAX_BUILTIN_COUNT);

    return &prog->builtin_values[idx];
}

void sn_program_add_builtin_fn(sn_program_t *prog, const char *str, sn_builtin_fn_t fn)
{
    sn_value_t *value = sn_program_add_builtin_value(prog, str);
    value->type = SN_VALUE_TYPE_BUILTIN_FN;
    value->builtin_fn = fn;
}

void sn_program_add_default_symbols(sn_program_t *prog)
{
    sn_symvec_init(&prog->builtin_idxs);
    prog->sn_fn = sn_program_default_symbol(prog, "fn");
    prog->sn_if = sn_program_default_symbol(prog, "if");

    sn_value_t *null = sn_program_add_builtin_value(prog, "null");
    null->type = SN_VALUE_TYPE_NULL;

    sn_program_add_builtin_fn(prog, "+", NULL);
    sn_program_add_builtin_fn(prog, "-", NULL);
}

sn_program_t *sn_program_create(const char *source, size_t size)
{
    sn_program_t *prog = calloc(1, sizeof *prog);
    prog->cur = source;
    prog->last = source + size;

    prog->symbol_tail = &prog->symbol_head;

    sn_program_add_default_symbols(prog);

    prog->msg = stderr;
    sn_cur_parse_sexpr_list(prog, &prog->expr);

    prog->cur = NULL;
    prog->last = NULL;
    return prog;
}

void sn_program_destroy(sn_program_t *prog)
{
    if (prog == NULL) {
        return;
    }

    free(prog);
}

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str)
{
    size_t len = strlen(str);
    if (len != sym->length) {
        return false;
    }

    return memcmp(sym->value, str, sym->length) == 0;
}
