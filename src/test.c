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

void test_parse_empty_list(void)
{
    char *src = "()";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_NULL(expr->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_empty_list_with_spaces(void)
{
    char *src = "  (  )  ";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_NULL(expr->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_nested_empty_list(void)
{
    char *src = "(())";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_NULL(expr->child_head->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_list_of_integers(void)
{
    char *src = "(1 2 3 4)";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);
    ASSERT_EQ(expr->child_head->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_list_of_integers_with_spaces(void)
{
    char *src = " ( 1   2    3     4  )   ";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);
    ASSERT_EQ(expr->child_head->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_list_of_some_negative_integers(void)
{
    char *src = "(-1 2 -3 4)";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, -1);
    ASSERT_EQ(expr->child_head->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, -3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_multiple_lists(void)
{
    char *src = "(1 (2) 3) ()";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);

    ASSERT_EQ(expr->child_head->next->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->next->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->child_head->vint, 2);

    ASSERT_EQ(expr->child_head->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);

    ASSERT_EQ(expr->next->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_NULL(expr->next->child_head);

    ASSERT_NULL(expr->next->next);
    sn_program_destroy(prog);
}

void test_parse_with_comments(void)
{
    char *src = ";; beginning comment\n"
                "(1 (2) ;; end of line\n"
                "3) ();; end of file";
    sn_program_t *prog = sn_program_create(src, strlen(src));

    sn_sexpr_t *expr = sn_program_test_get_first_sexpr(prog);
    ASSERT_EQ(expr->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);

    ASSERT_EQ(expr->child_head->next->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_EQ(expr->child_head->next->child_head->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->child_head->vint, 2);

    ASSERT_EQ(expr->child_head->next->next->type, SN_SEXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);

    ASSERT_EQ(expr->next->type, SN_SEXPR_TYPE_SEXPR);
    ASSERT_NULL(expr->next->child_head);

    ASSERT_NULL(expr->next->next);
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
    test_parse_empty_list();
    test_parse_empty_list_with_spaces();
    test_parse_nested_empty_list();
    test_parse_list_of_integers();
    test_parse_list_of_integers_with_spaces();
    test_parse_list_of_some_negative_integers();
    test_parse_multiple_lists();
    test_parse_with_comments();
    printf("PASSED\n");
    return 0;
}
