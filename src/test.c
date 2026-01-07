#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "snprogram.h"

#define ASSERT(x) assert(x)
#define ASSERT_EQ(x, y) ASSERT((x) == (y))
#define ASSERT_NULL(x) ASSERT_EQ(x, NULL)

void test_prog_create_destroy(void)
{
    char *src = "1234";
    sn_program_t *prog = sn_program_create(src, strlen(src));
    ASSERT(prog);
    sn_program_destroy(prog);
}

void test_parse_int(void)
{
    char *src = "1234";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *first = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(first->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_leading_whitespace_int(void)
{
    char *src = "  1234";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *first = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(first->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_negative_int(void)
{
    char *src = "-1234";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *first = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(first->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, -1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_single_digit(void)
{
    char *src = "4";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *first = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(first->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 4);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_single_negative_digit(void)
{
    char *src = "-4";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *first = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(first->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, -4);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_two_integers(void)
{
    char *src = "123 456";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->vint, 123);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->vint, 456);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

int main(int argc, char **argv)
{
    test_prog_create_destroy();
    test_parse_int();
    test_parse_negative_int();
    test_parse_leading_whitespace_int();
    test_parse_single_digit();
    test_parse_single_negative_digit();
    test_parse_two_integers();
    printf("PASSED\n");
    return 0;
}
