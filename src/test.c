#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "snprogram.h"

#define ASSERT(x) assert(x)
#define ASSERT_EQ(x, y) ASSERT((x) == (y))
#define ASSERT_NULL(x) ASSERT_EQ(x, NULL)
#define ASSERT_OK(x) ASSERT_EQ(x, SN_SUCCESS)

#define ASSERT_EQ_INT(x, y) ASSERT((x).type == SN_VALUE_TYPE_INTEGER && (x).i == (y))
#define ASSERT_NULL_TYPE(x) ASSERT((x).type == SN_VALUE_TYPE_NULL)

void test_prog_create_destroy(void)
{
    char *src = "1234";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT(prog);
    sn_program_destroy(prog);
}

void test_parse_int(void)
{
    char *src = "1234";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *first = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(first->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_leading_whitespace_int(void)
{
    char *src = "  1234";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *first = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(first->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_negative_int(void)
{
    char *src = "-1234";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *first = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(first->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, -1234);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_single_digit(void)
{
    char *src = "4";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *first = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(first->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, 4);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_single_negative_digit(void)
{
    char *src = "-4";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *first = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(first->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(first->vint, -4);
    ASSERT_NULL(first->next);

    sn_program_destroy(prog);
}

void test_parse_two_integers(void)
{
    char *src = "123 456";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->vint, 123);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->vint, 456);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_empty_list(void)
{
    char *src = "()";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_empty_list_with_spaces(void)
{
    char *src = "  (  )  ";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_nested_empty_list(void)
{
    char *src = "(())";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->child_head->child_head);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_list_of_integers(void)
{
    char *src = "(1 2 3 4)";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_list_of_integers_with_spaces(void)
{
    char *src = " ( 1   2    3     4  )   ";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_list_of_some_negative_integers(void)
{
    char *src = "(-1 2 -3 4)";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, -1);
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, -3);
    ASSERT_EQ(expr->child_head->next->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->next->vint, 4);
    ASSERT_NULL(expr->child_head->next->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_multiple_lists(void)
{
    char *src = "(1 (2) 3) ()";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);

    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->next->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->child_head->vint, 2);

    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);

    ASSERT_EQ(expr->next->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->next->child_head);

    ASSERT_NULL(expr->next->next);
    sn_program_destroy(prog);
}

void test_parse_with_comments(void)
{
    char *src = ";; beginning comment\n"
                "(1 (2) ;; end of line\n"
                "3) ();; end of file";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);

    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->next->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->child_head->vint, 2);

    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);

    ASSERT_EQ(expr->next->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->next->child_head);

    ASSERT_NULL(expr->next->next);
    sn_program_destroy(prog);
}

void test_parse_curlys(void)
{
    char *src = "{2 1 3}";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->vint, 1);
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->vint, 2);
    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 3);
    ASSERT_NULL(expr->child_head->next->next->next);
    sn_program_destroy(prog);
}

void test_parse_symbol(void)
{
    char *src = "hello";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->sym, src));
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_two_symbols(void)
{
    char *src = "hello world";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->sym, "hello"));

    ASSERT_EQ(expr->next->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->next->sym, "world"));
    ASSERT_NULL(expr->next->next);

    sn_program_destroy(prog);
}

void test_parse_fancy_symbols(void)
{
    char *src = "hello? <";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->sym, "hello?"));

    ASSERT_EQ(expr->next->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->next->sym, "<"));
    ASSERT_NULL(expr->next->next);

    sn_program_destroy(prog);
}

void test_parse_repeat_symbols(void)
{
    char *src = "hello? hello?";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));

    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->sym, "hello?"));

    ASSERT_EQ(expr->next->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->sym, expr->next->sym);
    ASSERT_NULL(expr->next->next);

    sn_program_destroy(prog);
}

void test_parse_list_and_symbols(void)
{
    char *src = "(if {a > 0} a (- a))";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->next);

    expr = expr->child_head;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->sym, "if"));

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->child_head->sym, ">"));

    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_SYMBOL);
    sn_symbol_t *a = expr->child_head->next->sym;
    ASSERT(sn_symbol_equals_string(a, "a"));

    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 0);
    ASSERT_NULL(expr->child_head->next->next->next);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->sym, a);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->child_head->sym, "-"));
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->child_head->next->sym, a);
    ASSERT_NULL(expr->child_head->next->next);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_parse_list_symbols_comments(void)
{
    char *src = "(if {a > 0} ;; check if a is positive\n"
                "  a ;; if so, return a\n"
                "  (- a)) ;; otherwise, negate a\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_expr_t *expr = sn_program_test_get_first_expr(prog);
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_NULL(expr->next);

    expr = expr->child_head;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->sym, prog->sn_if);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->child_head->sym, ">"));

    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_SYMBOL);
    sn_symbol_t *a = expr->child_head->next->sym;
    ASSERT(sn_symbol_equals_string(a, "a"));

    ASSERT_EQ(expr->child_head->next->next->type, SN_EXPR_TYPE_INTEGER);
    ASSERT_EQ(expr->child_head->next->next->vint, 0);
    ASSERT_NULL(expr->child_head->next->next->next);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->sym, a);

    expr = expr->next;
    ASSERT_EQ(expr->type, SN_EXPR_TYPE_LIST);
    ASSERT_EQ(expr->child_head->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT(sn_symbol_equals_string(expr->child_head->sym, "-"));
    ASSERT_EQ(expr->child_head->next->type, SN_EXPR_TYPE_SYMBOL);
    ASSERT_EQ(expr->child_head->next->sym, a);
    ASSERT_NULL(expr->child_head->next->next);
    ASSERT_NULL(expr->next);

    sn_program_destroy(prog);
}

void test_eval_literal(void)
{
    char *src = "123\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, 123);
    sn_program_destroy(prog);

    src = "-123\n";
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, -123);
    sn_program_destroy(prog);
}

void test_eval_sum(void)
{
    char *src = "(+ 1 2 3)\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, 6);
    sn_program_destroy(prog);

    src = "{10 - 5}\n";
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, 5);
    sn_program_destroy(prog);
}

void test_eval_nested(void)
{
    char *src = "(+ 1 2 3 (- -4))\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, 10);
    sn_program_destroy(prog);
}

void test_variable(void)
{
    char *src = "(let x 12)\n"
                "(let y (+ x 1))\n"
                "(+ x y)\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_EQ_INT(val, 25);
    sn_program_destroy(prog);
}

void test_null(void)
{
    char *src = "null\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_NULL_TYPE(val);
    sn_program_destroy(prog);
}

void test_println(void)
{
    char *src = "(println (+ 1 2) (+ 2 3))\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_value_t val = { SN_VALUE_TYPE_INVALID };
    ASSERT_OK(sn_program_run(prog, &val));
    ASSERT_NULL_TYPE(val);
    sn_program_destroy(prog);
}

void test_parse_error(void)
{
    char *src = "(\n";
    sn_program_t *prog = NULL;
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_UNEXPECTED_END_OF_INPUT);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);

    src = ")\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_EXTRA_CHARS_AT_END_OF_INPUT);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);

    src = "(}\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_EXPECTED_EXPR_CLOSE);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);

    src = "(\n"
          "(x)\n"
          "({a b c d}))\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);

    src = "\n\n1234x";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INVALID_INTEGER_LITERAL);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);

    src = "var'";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INVALID_SYMBOL_NAME);
    sn_program_write_error(prog, stderr);
    sn_program_destroy(prog);
}

void test_build_error(void)
{
    char *src = "(let a 1)\n"
                "  (let b a a)\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_EQ(sn_program_get_status(prog), SN_SUCCESS);
    ASSERT_EQ(sn_program_build(prog), SN_ERROR_LET_EXPR_NOT_3_ITEMS);
    sn_program_write_error(prog, stderr);
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
    test_parse_curlys();
    test_parse_symbol();
    test_parse_two_symbols();
    test_parse_fancy_symbols();
    test_parse_repeat_symbols();
    test_parse_list_and_symbols();
    test_parse_list_symbols_comments();
    test_eval_literal();
    test_eval_sum();
    test_eval_nested();
    test_variable();
    test_null();
    test_println();
    test_parse_error();
    test_build_error();
    printf("PASSED\n");
    return 0;
}
