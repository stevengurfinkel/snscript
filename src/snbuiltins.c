#include "snprogram.h"

bool sn_add(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_INTEGER;
    ret->i = 0;
    for (int i = 0; i < arg_count; i++) {
        if (args[i].type != SN_VALUE_TYPE_INTEGER) {
            return false;
        }
        ret->i += args[i].i;
    }
    return true;
}

bool sn_sub(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_INTEGER;
    for (int i = 0; i < arg_count; i++) {
        if (args[i].type != SN_VALUE_TYPE_INTEGER) {
            return false;
        }
    }

    if (arg_count == 1) {
        ret->i = -args[0].i;
    }
    else if (arg_count == 2) {
        ret->i = args[0].i - args[1].i;
    }
    else {
        return false;
    }

    return true;
}
