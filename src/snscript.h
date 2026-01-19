#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum sn_error_en
{
    SN_SUCCESS = 0,
    SN_ERROR_UNEXPECTED_END_OF_INPUT,
    SN_ERROR_EXPECTED_EXPR_CLOSE,
    SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS,
    SN_ERROR_EXTRA_CHARS_AT_END_OF_INPUT,
    SN_ERROR_INVALID_INTEGER_LITERAL,
    SN_ERROR_INVALID_SYMBOL_NAME,
    SN_ERROR_LET_EXPR_NOT_3_ITEMS,
    SN_ERROR_LET_EXPR_BAD_DEST,
    SN_ERROR_FN_EXPR_TOO_SHORT,
    SN_ERROR_FN_PROTO_NOT_LIST,
    SN_ERROR_FN_PROTO_COTAINS_NON_SYMBOLS,
    SN_ERROR_IF_EXPR_INVALID_LENGTH,
    SN_ERROR_EMPTY_EXPR,
    SN_ERROR_NESTED_FN_EXPR,
    SN_ERROR_NESTED_LET_EXPR,
    SN_ERROR_UNDECLARED,
    SN_ERROR_REDECLARED,
    SN_ERROR_CALLEE_NOT_A_FN,
    SN_ERROR_INVALID_PARAMS_TO_FN,
    SN_ERROR_WRONG_VALUE_TYPE,
    SN_ERROR_GENERIC = 0x7FFFFFFF
} sn_error_t;

typedef struct sn_program_st sn_program_t;
typedef struct sn_value_st sn_value_t;

sn_error_t sn_program_create(sn_program_t **program_out, const char *source, size_t size);
void sn_program_destroy(sn_program_t *prog);
sn_error_t sn_program_run(sn_program_t *prog, sn_value_t *value_out);

const char *sn_error_str(sn_error_t status);
void sn_program_write_error(sn_program_t *prog, FILE *stream);

sn_value_t *sn_value_create(void);
void sn_value_destroy(sn_value_t *value);
sn_error_t sn_value_as_integer(sn_value_t *value, int64_t *i_out);
bool sn_value_is_null(sn_value_t *value);
