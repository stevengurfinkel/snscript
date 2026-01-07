#include <stdbool.h>
#include <stdint.h>
#include "snscript.h"

struct sn_symbol_st
{
    size_t length;
    char *value;

    sn_symbol_t *next;
};

struct sn_program_st
{
    sn_sexpr_t *expr_head;
    sn_sexpr_t **expr_tail;

    sn_symbol_t *symbol_head;
    sn_symbol_t **symbol_tail;

    sn_symbol_t src;
};

struct sn_sexpr_st
{
    sn_sexpr_type_t type;
    int64_t vint;
    sn_symbol_t sym;
    sn_sexpr_t *child_head;
    sn_sexpr_t **child_tail;
    sn_sexpr_t *next;
};

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str);
sn_sexpr_t *sn_program_test_get_first_sexpr(sn_program_t *prog);
