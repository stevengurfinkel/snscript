#include <stdlib.h>
#include "snscript_internal.h"

sn_value_type_t sn_unified_type(sn_value_type_t type)
{
    return type == SN_VALUE_TYPE_BUILTIN_FN ? SN_VALUE_TYPE_USER_FN : type;
}

sn_error_t sn_equals(sn_value_t *ret, int arg_count, const sn_value_t *args)
{
    ret->type = SN_VALUE_TYPE_BOOLEAN;

    if (arg_count != 2) {
        return SN_ERROR_WRONG_ARG_COUNT_IN_CALL;
    }

    // consider user and builtin functions as the same type
    if (sn_unified_type(args[0].type) != sn_unified_type(args[1].type)) {
        return SN_ERROR_WRONG_VALUE_TYPE;
    }

    // a builtin function will never equal a user function
    if (args[0].type != args[0].type) {
        ret->i = false;
        return SN_SUCCESS;
    }

    switch (args[0].type) {
        case SN_VALUE_TYPE_INVALID:
            abort();
            return SN_ERROR_GENERIC;
        case SN_VALUE_TYPE_NULL:
            ret->i = true;
            break;
        case SN_VALUE_TYPE_INTEGER:
        case SN_VALUE_TYPE_BOOLEAN:
            ret->i = (args[0].i == args[1].i);
            break;
        case SN_VALUE_TYPE_USER_FN:
            ret->i = (args[0].user_fn == args[1].user_fn);
            break;
        case SN_VALUE_TYPE_BUILTIN_FN:
            ret->i = (args[0].builtin_fn == args[1].builtin_fn);
            break;
    }

    return SN_SUCCESS;
}

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
            case SN_VALUE_TYPE_BOOLEAN:
                printf("%s", args[i].i ? "true" : "false");
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
