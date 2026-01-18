#include "snprogram.h"

sn_error_t sn_add(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_INTEGER;
    ret->i = 0;
    for (int i = 0; i < arg_count; i++) {
        if (args[i].type != SN_VALUE_TYPE_INTEGER) {
            return SN_ERROR_INVALID_PARAMS_TO_FN;
        }
        ret->i += args[i].i;
    }
    return SN_SUCCESS;
}

sn_error_t sn_sub(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_INTEGER;
    for (int i = 0; i < arg_count; i++) {
        if (args[i].type != SN_VALUE_TYPE_INTEGER) {
            return SN_ERROR_INVALID_PARAMS_TO_FN;
        }
    }

    if (arg_count == 1) {
        ret->i = -args[0].i;
    }
    else if (arg_count == 2) {
        ret->i = args[0].i - args[1].i;
    }
    else {
        return SN_ERROR_INVALID_PARAMS_TO_FN;
    }

    return SN_SUCCESS;
}

sn_error_t sn_println(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_NULL;
    for (int i = 0; i < arg_count; i++) {
        if (i != 0) {
            putchar(' ');
        }
        switch (args[i].type) {
            case SN_VALUE_TYPE_INVALID:
                return SN_ERROR_INVALID_PARAMS_TO_FN;
            case SN_VALUE_TYPE_NULL:
                printf("null");
                break;
            case SN_VALUE_TYPE_INTEGER:
                printf("%ld", args[i].i);
                break;
            case SN_VALUE_TYPE_USER_FN:
                printf("<user fn>");
                break;
            case SN_VALUE_TYPE_BUILTIN_FN:
                printf("<builtin fn>");
                break;
        }
    }
    printf("\n");
    return SN_SUCCESS;
}
