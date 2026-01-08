#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "snscript.h"

#define SN_PROGRAM_MAX_BUILTIN_COUNT 64

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

    // special forms
    sn_symbol_t *sn_fn;
    sn_symbol_t *sn_if;

    // bulitin functions, null
    sn_symvec_t builtin_idxs;
    sn_value_t builtin_values[SN_PROGRAM_MAX_BUILTIN_COUNT];
};

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
