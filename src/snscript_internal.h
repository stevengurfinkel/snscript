#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "snscript.h"

typedef enum sn_expr_type_en
{
    SN_EXPR_TYPE_INVALID = 0,
    SN_EXPR_TYPE_INTEGER,
    SN_EXPR_TYPE_SYMBOL,
    SN_EXPR_TYPE_LIST,
} sn_expr_type_t;

typedef enum sn_value_type_en
{
    SN_VALUE_TYPE_INVALID,
    SN_VALUE_TYPE_NULL,
    SN_VALUE_TYPE_INTEGER,
    SN_VALUE_TYPE_BOOLEAN,
    SN_VALUE_TYPE_USER_FN,
    SN_VALUE_TYPE_BUILTIN_FN,
} sn_value_type_t;

typedef struct sn_symbol_st sn_symbol_t;
typedef struct sn_expr_st sn_expr_t;
typedef struct sn_func_st sn_func_t;
typedef struct sn_scope_st sn_scope_t;
typedef struct sn_const_st sn_const_t;
typedef struct sn_env_st sn_env_t;
typedef sn_error_t (*sn_builtin_fn_t)(sn_value_t *ret, int arg_count, const sn_value_t *args);

struct sn_env_st
{
    sn_value_t *globals;
    sn_value_t *locals;
};

struct sn_value_st
{
    sn_value_type_t type;
    union {
        int64_t i;
        sn_func_t *user_fn;
        sn_builtin_fn_t builtin_fn;
    };
};

typedef enum sn_rtype_st
{
    SN_RTYPE_INVALID,

    SN_RTYPE_LET_KEYW,
    SN_RTYPE_FN_KEYW,
    SN_RTYPE_IF_KEYW,
    SN_RTYPE_DO_KEYW,

    SN_RTYPE_LET_EXPR,
    SN_RTYPE_FN_EXPR,
    SN_RTYPE_IF_EXPR,
    SN_RTYPE_DO_EXPR,

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

typedef enum sn_scope_type_en
{
    SN_SCOPE_TYPE_INVALID,
    SN_SCOPE_TYPE_GLOBAL,
    SN_SCOPE_TYPE_LOCAL,
} sn_scope_type_t;

typedef struct sn_ref_st
{
    sn_scope_type_t type;
    int index;
} sn_ref_t;

struct sn_const_st
{
    sn_value_t value;
    int idx;
    sn_const_t *next;
};

struct sn_scope_st
{
    sn_const_t *head_const;
    sn_scope_t *parent;
    sn_expr_t *decl_head;
    int decl_count;
};

struct sn_func_st
{
    int param_count;
    sn_scope_t scope;
    sn_symbol_t *name;
    sn_expr_t *body;
};

struct sn_expr_st
{
    sn_expr_type_t type;
    sn_rtype_t rtype;
    int64_t vint;
    sn_symbol_t *sym;
    size_t child_count;
    sn_expr_t *child_head;
    sn_expr_t *next;

    sn_ref_t ref;
    sn_expr_t *next_decl;
    sn_program_t *prog;
    int line;
    int col;
};

struct sn_program_st
{
    int error_line;
    int error_col;
    sn_symbol_t *error_sym;

    sn_expr_t expr;

    sn_symbol_t *symbol_head;
    sn_symbol_t **symbol_tail;

    const char *start;
    const char *cur;
    const char *last;
    int cur_line;
    int cur_col;

    // special forms
    sn_symbol_t *sn_let;
    sn_symbol_t *sn_fn;
    sn_symbol_t *sn_if;
    sn_symbol_t *sn_do;

    sn_scope_t globals;
};

extern sn_value_t sn_null;

sn_error_t sn_expr_error(sn_expr_t *expr, sn_error_t error);
sn_error_t sn_expr_eval(sn_expr_t *expr, sn_env_t *env, sn_value_t *val_out);

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str);
sn_expr_t *sn_program_test_get_first_expr(sn_program_t *prog);
sn_symbol_t *sn_program_get_symbol(sn_program_t *prog, const char *start, const char *end);
sn_error_t sn_program_parse(sn_program_t *prog);

sn_error_t sn_scope_add_var(sn_scope_t *scope, sn_expr_t *expr);
sn_error_t sn_scope_find_var(sn_scope_t *scope, sn_symbol_t *name, sn_ref_t *ref);
sn_value_t *sn_scope_create_const(sn_scope_t *scope, const sn_ref_t *ref);
void sn_scope_init_consts(sn_scope_t *scope, sn_value_t *values);

void sn_symvec_init(sn_symvec_t *symvec);
int sn_symvec_idx(sn_symvec_t *symvec, sn_symbol_t *name);
int sn_symvec_append(sn_symvec_t *symvec, sn_symbol_t *name);
void sn_symvec_deinit(sn_symvec_t *symvec);

sn_error_t sn_is_int(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_is_fn(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_is_null(sn_value_t *ret, int arg_count, const sn_value_t *args);

sn_error_t sn_not_equals(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_not(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_equals(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_add(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_sub(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_mul(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_div(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_mod(sn_value_t *ret, int arg_count, const sn_value_t *args);
sn_error_t sn_println(sn_value_t *ret, int arg_count, const sn_value_t *args);
