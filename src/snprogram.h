#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "snscript.h"

struct sn_symbol_st
{
    size_t length;
    sn_symbol_t *next;
    char value[];
};

struct sn_sexpr_st
{
    sn_sexpr_type_t type;
    int64_t vint;
    sn_symbol_t *sym;
    size_t child_count;
    sn_sexpr_t *child_head;
    sn_sexpr_t *next;
};

struct sn_program_st
{
    sn_sexpr_t expr;

    sn_symbol_t *symbol_head;
    sn_symbol_t **symbol_tail;

    const char *cur;
    const char *last;

    FILE *msg;

    sn_symbol_t *sn_fn;
    sn_symbol_t *sn_if;
    sn_symbol_t *sn_plus;
    sn_symbol_t *sn_minus;
};

bool sn_symbol_equals_string(sn_symbol_t *sym, const char *str);
sn_sexpr_t *sn_program_test_get_first_sexpr(sn_program_t *prog);

bool sn_program_compile(sn_program_t *prog);

