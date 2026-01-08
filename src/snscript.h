#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum sn_sexpr_type_en
{
    SN_SEXPR_TYPE_INVALID = 0,
    SN_SEXPR_TYPE_INTEGER,
    SN_SEXPR_TYPE_SYMBOL,
    SN_SEXPR_TYPE_SEXPR,
} sn_sexpr_type_t;

typedef enum sn_value_type_en
{
    SN_VALUE_TYPE_INVALID,
    SN_VALUE_TYPE_NULL,
    SN_VALUE_TYPE_INTEGER,
    SN_VALUE_TYPE_USER_FN,
    SN_VALUE_TYPE_BUILTIN_FN,
} sn_value_type_t;

typedef struct sn_program_st sn_program_t;
typedef struct sn_symbol_st sn_symbol_t;
typedef struct sn_sexpr_st sn_sexpr_t;
typedef struct sn_value_st sn_value_t;
typedef struct sn_user_fn_st sn_user_fn_t;
typedef bool (*sn_builtin_fn_t)(sn_value_t *ret, int arg_count, const sn_value_t *args);
typedef struct sn_scope_st sn_scope_t;
typedef struct sn_var_st sn_var_t;

sn_program_t *sn_program_create(const char *source, size_t size);
void sn_program_destroy(sn_program_t *prog);

struct sn_value_st
{
    sn_value_type_t type;
    union {
        int64_t i;
        sn_user_fn_t *user_fn;
        sn_builtin_fn_t builtin_fn;
    };
};
