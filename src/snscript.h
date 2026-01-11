#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum sn_expr_type_en
{
    SN_SEXPR_TYPE_INVALID = 0,
    SN_SEXPR_TYPE_INTEGER,
    SN_SEXPR_TYPE_SYMBOL,
    SN_SEXPR_TYPE_SEXPR,
} sn_expr_type_t;

typedef enum sn_value_type_en
{
    SN_VALUE_TYPE_INVALID,
    SN_VALUE_TYPE_NULL,
    SN_VALUE_TYPE_INTEGER,
    SN_VALUE_TYPE_USER_FN,
    SN_VALUE_TYPE_BUILTIN_FN,
} sn_value_type_t;

typedef enum sn_error_en
{
    SN_SUCCESS = 0,
    SN_ERROR_UNEXPECTED_END_OF_INPUT,
    SN_ERROR_EXPECTED_EXPR_CLOSE,
    SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS,
    SN_ERROR_EXTRA_CHARS_AT_END_OF_INPUT,
    SN_ERROR_INVALID_INTEGER_LITERAL,
    SN_ERROR_INVALID_SYMBOL_NAME,
    SN_ERROR_GENERIC = 0x7FFFFFFF
} sn_error_t;

typedef struct sn_program_st sn_program_t;
typedef struct sn_symbol_st sn_symbol_t;
typedef struct sn_expr_st sn_expr_t;
typedef struct sn_value_st sn_value_t;
typedef struct sn_user_fn_st sn_user_fn_t;
typedef bool (*sn_builtin_fn_t)(sn_value_t *ret, int arg_count, const sn_value_t *args);

sn_program_t *sn_program_create(const char *source, size_t size);
void sn_program_destroy(sn_program_t *prog);
sn_value_t sn_program_run(sn_program_t *prog);

const char *sn_error_str(sn_error_t status);
void sn_program_write_error(sn_program_t *prog, FILE *stream);

void sn_value_print(sn_value_t value, FILE *stream);

struct sn_value_st
{
    sn_value_type_t type;
    union {
        int64_t i;
        sn_user_fn_t *user_fn;
        sn_builtin_fn_t builtin_fn;
    };
};
