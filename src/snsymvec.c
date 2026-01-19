#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "snscript_internal.h"


void sn_symvec_resize(sn_symvec_t *symvec, int capacity)
{
    assert(capacity > symvec->capacity);
    symvec->capacity = capacity;
    symvec->names = reallocarray(symvec->names, capacity, sizeof symvec->names[0]);
}

void sn_symvec_init(sn_symvec_t *symvec)
{
    sn_symvec_resize(symvec, 1);
}

int sn_symvec_idx(sn_symvec_t *symvec, sn_symbol_t *name)
{
    for (int i = 0; i < symvec->count; i++) {
        if (symvec->names[i] == name) {
            return i;
        }
    }

    return -1;
}

int sn_symvec_append(sn_symvec_t *symvec, sn_symbol_t *name)
{
    // check for duplicate declaration
    if (sn_symvec_idx(symvec, name) >= 0) {
        return -1;
    }

    if (symvec->count == symvec->capacity) {
        sn_symvec_resize(symvec, 2 * symvec->capacity);
    }

    int idx = symvec->count++;
    symvec->names[idx] = name;
    return idx;
}

void sn_symvec_deinit(sn_symvec_t *symvec)
{
    free(symvec->names);
    memset(symvec, '\0', sizeof *symvec);
}
