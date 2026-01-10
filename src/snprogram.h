#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "snscript.h"

typedef enum sn_rtype_st
{
    SN_RTYPE_INVALID,

    SN_RTYPE_LET_KEYW,
    SN_RTYPE_FN_KEYW,
    SN_RTYPE_IF_KEYW,

    SN_RTYPE_LET_EXPR,
    SN_RTYPE_FN_EXPR,
    SN_RTYPE_IF_EXPR,

    SN_RTYPE_VAR,
    SN_RTYPE_LITERAL,
    SN_RTYPE_CALL,

    SN_RTYPE_PROGRAM,

} sn_rtype_t;

struct sn_symbol_st
{
    size_t length;
    sn_symbol_t *next;
    char value[];
};

typedef struct sn_symvec_st
{
    int capacity;
    int count;
    sn_symbol_t **names;
} sn_symvec_t;

typedef enum sn_scope_en
{
    SN_SCOPE_GLOBAL,
    SN_SCOPE_LOCAL,
} sn_scope_t;

typedef struct sn_ref_st
{
    sn_scope_t scope;
    int index;
} sn_ref_t;

struct sn_sexpr_st
{
    sn_sexpr_type_t type;
    sn_rtype_t rtype;
    int64_t vint;
    sn_symbol_t *sym;
    size_t child_count;
    sn_sexpr_t *child_head;
    sn_sexpr_t *next;

    sn_ref_t ref;
    sn_program_t *prog;
};

typedef struct sn_builtin_value_st sn_builtin_value_t;
struct sn_builtin_value_st
{
    sn_value_t value;
    sn_builtin_value_t *next;
};

struct sn_program_st
{
    sn_sexpr_t expr;

    sn_symbol_t *symbol_head;
    sn_symbol_t **symbol_tail;

    const char *cur;
    const char *last;

    FILE *msg;

    // special forms
    sn_symbol_t *sn_let;
    sn_symbol_t *sn_fn;
    sn_symbol_t *sn_if;

    // bulitin functions, null
    int builtin_count;
    sn_builtin_value_t *builtin_head;

    sn_symvec_t global_idxs;
    sn_value_t *global_values;
};

extern sn_value_t sn_null;

void sn_program_build(sn_program_t *prog);

sn_value_t sn_program_eval_expr(sn_program_t *prog, sn_sexpr_t *expr);

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str);
sn_sexpr_t *sn_program_test_get_first_sexpr(sn_program_t *prog);
sn_symbol_t *sn_program_get_symbol(sn_program_t *prog, const char *start, const char *end);
void sn_cur_parse_sexpr_list(sn_program_t *prog, sn_sexpr_t *expr);

void sn_symvec_init(sn_symvec_t *symvec);
int sn_symvec_idx(sn_symvec_t *symvec, sn_symbol_t *name);
int sn_symvec_append(sn_symvec_t *symvec, sn_symbol_t *name);
void sn_symvec_deinit(sn_symvec_t *symvec);

bool sn_add(sn_value_t *ret, int arg_count, const sn_value_t *args);
bool sn_sub(sn_value_t *ret, int arg_count, const sn_value_t *args);
