#pragma once

typedef enum sn_sexpr_type_en
{
    SN_SEXPR_TYPE_INVALID = 0,
    SN_SEXPR_TYPE_INTEGER,
    SN_SEXPR_TYPE_SYMBOL,
    SN_SEXPR_TYPE_SEXPR,
} sn_sexpr_type_t;

typedef struct sn_program_st sn_program_t;
typedef struct sn_symbol_st sn_symbol_t;
typedef struct sn_sexpr_st sn_sexpr_t;

sn_program_t *sn_program_create(const char *source, size_t size);
void sn_program_destroy(sn_program_t *prog);
