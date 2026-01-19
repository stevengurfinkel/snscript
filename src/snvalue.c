#include <stdlib.h>
#include "snscript_internal.h"

sn_value_t *sn_value_create(void)
{
    sn_value_t *out = calloc(1, sizeof *out);
    out->type = SN_VALUE_TYPE_INVALID;
    return out;
}

void sn_value_destroy(sn_value_t *value)
{
    if (value == NULL) {
        return;
    }

    free(value);
}

sn_error_t sn_value_as_integer(sn_value_t *value, int64_t *i_out)
{
    if (value->type != SN_VALUE_TYPE_INTEGER) {
        return SN_ERROR_WRONG_VALUE_TYPE;
    }

    *i_out = value->i;
    return SN_SUCCESS;
}

bool sn_value_is_null(sn_value_t *value)
{
    return value->type == SN_VALUE_TYPE_NULL;
}
