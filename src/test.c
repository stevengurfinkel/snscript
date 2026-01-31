#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "snscript_internal.h"

#define ASSERT(x) assert(x)
#define ASSERT_EQ(x, y) ASSERT((x) == (y))
#define ASSERT_NULL(x) ASSERT_EQ(x, NULL)
#define ASSERT_OK(x) ASSERT_EQ(x, SN_SUCCESS)

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

int64_t ival(sn_value_t *v)
{
    int64_t i = 0;
    ASSERT_OK(sn_value_as_integer(v, &i));
    return i;
}

bool bval(sn_value_t *v)
{
    bool b = false;
    ASSERT_OK(sn_value_as_boolean(v, &b));
    return b;
}

void test_eval_literal(void)
{
    char *src = "123\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), 123);
    sn_program_destroy(prog);

    src = "-123\n";
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), -123);
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void test_eval_sum(void)
{
    char *src = "(+ 1 2 3)\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), 6);
    sn_program_destroy(prog);

    src = "{10 - 5}\n";
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), 5);
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void test_eval_nested(void)
{
    char *src = "(+ 1 2 3 (- -4))\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), 10);
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void test_variable(void)
{
    char *src = "(let x 12)\n"
                "(let y (+ x 1))\n"
                "(+ x y)\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT_EQ(ival(val), 25);
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void test_null(void)
{
    char *src = "null\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT(sn_value_is_null(val));
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void test_println(void)
{
    char *src = "(println (+ 0 1) (+ 1 1) (+ 1 2))\n";
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_value_t *val = sn_value_create();
    ASSERT_OK(sn_program_run(prog, val));
    ASSERT(sn_value_is_null(val));
    sn_value_destroy(val);
    sn_program_destroy(prog);
}

void error_check(sn_program_t *prog, int expect_line, int expect_col, const char *expect_str)
{
    int actual_line;
    int actual_col;
    const char *actual_str;
    sn_program_error_pos(prog, &actual_line, &actual_col);
    sn_program_error_symbol(prog, &actual_str);
    ASSERT_EQ(expect_line, actual_line);
    ASSERT_EQ(expect_col, actual_col);
    if (expect_str == NULL) {
        ASSERT_EQ(actual_str, NULL);
    }
    else {
        ASSERT_EQ(strcmp(actual_str, expect_str), 0);
    }
}

void test_parse_error(void)
{
    char *src = "(\n";
    sn_program_t *prog = NULL;
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_UNEXPECTED_END_OF_INPUT);
    error_check(prog, 2, 1, NULL);
    sn_program_destroy(prog);

    src = ")\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_EXTRA_CHARS_AT_END_OF_INPUT);
    error_check(prog, 1, 1, NULL);
    sn_program_destroy(prog);

    src = "(}\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_EXPECTED_EXPR_CLOSE);
    error_check(prog, 1, 2, NULL);
    sn_program_destroy(prog);

    src = "(\n"
          "(x)\n"
          "({a b c d}))\n";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INFIX_EXPR_NOT_3_ELEMENTS);
    error_check(prog, 3, 2, NULL);
    sn_program_destroy(prog);

    src = "\n\n1234x";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INVALID_INTEGER_LITERAL);
    error_check(prog, 3, 5, NULL);
    sn_program_destroy(prog);

    src = "var'";
    ASSERT_EQ(sn_program_create(&prog, src, strlen(src)), SN_ERROR_INVALID_SYMBOL_NAME);
    error_check(prog, 1, 4, NULL);
    sn_program_destroy(prog);
}

void
error_build(sn_error_t err_code,
            int err_line,
            int err_col,
            const char *err_sym,
            const char *src)
{
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    sn_error_t status = sn_program_build(prog);
    if (status != err_code) {
        fprintf(stderr, "sn_program_build returned %s\n", sn_error_str(status));
        abort();
    }
    if (err_code != SN_SUCCESS) {
        error_check(prog, err_line, err_col, err_sym);
    }
    sn_program_destroy(prog);
}


void test_build_error(void)
{
    error_build(SN_ERROR_EXPR_NOT_3_ITEMS, 2, 3, NULL,
                "(let a 1)\n"
                "  (let b a a)\n");

    error_build(SN_ERROR_EXPR_BAD_DEST, 3, 6, NULL,
                "(let a 1)\n"
                "(let b 2)\n"
                "(let (+ a b) 3)\n");

    error_build(SN_ERROR_UNDECLARED, 1, 8, "a",
                "(let a a)\n");

    // function that returns NULL -> success
    error_build(SN_SUCCESS, 0, 0, NULL,
                "(fn (foo) null)\n");

    // function mus have at least three elements
    error_build(SN_ERROR_FN_EXPR_TOO_SHORT, 2, 2, NULL,
                "(let x +)\n"
                " (fn)\n");

    error_build(SN_ERROR_FN_EXPR_TOO_SHORT, 1, 1, NULL,
                "(fn (foo a b c))\n");

    error_build(SN_ERROR_FN_EXPR_TOO_SHORT, 1, 1, NULL,
                "(fn (foo))\n");

    error_build(SN_ERROR_FN_PROTO_NOT_LIST, 2, 1, NULL,
                "\n"
                "(fn bar baz)\n");

    // function name must not already be declared
    error_build(SN_ERROR_REDECLARED, 2, 6, "foo",
                "(let foo 1)\n"
                "(fn (foo) 1)\n");

    // function prototype must be a list of symbols
    error_build(SN_ERROR_FN_PROTO_COTAINS_NON_SYMBOLS, 1, 1, NULL,
                "(fn (1 2) null)\n");

    error_build(SN_ERROR_FN_PROTO_COTAINS_NON_SYMBOLS, 1, 1, NULL,
                "(fn (foo (+ 1 2)) null)\n");

    error_build(SN_ERROR_FN_PROTO_COTAINS_NON_SYMBOLS, 1, 1, NULL,
                "(fn ((null)) null)\n");

    // if-statement with only a 'true' arm
    error_build(SN_SUCCESS, 0, 0, NULL,
                "(let x 0)\n"
                "(let y (if x 0))\n");

    // if-statement with both a 'true' and 'false' arm
    error_build(SN_SUCCESS, 0, 0, NULL,
                "(let x 0)\n"
                "(let y (if x 0 1))\n");

    // no arms
    error_build(SN_ERROR_IF_EXPR_INVALID_LENGTH, 2, 8, NULL,
                "(let x 0)\n"
                "(let y (if x))\n");

    // three arms
    error_build(SN_ERROR_IF_EXPR_INVALID_LENGTH, 2, 8, NULL,
                "(let x 0)\n"
                "(let y (if x 0 1 2))\n");

    // empty epxression
    error_build(SN_ERROR_EMPTY_EXPR, 1, 1, NULL,
                "()\n");

    error_build(SN_ERROR_EMPTY_EXPR, 2, 10, NULL,
                "(let a\n"
                "     (if () - +))\n");

    // nested functions aren't allowed
    error_build(SN_ERROR_NESTED_FN_EXPR, 1, 8, NULL,
                "(let a (fn (foo) null))\n");

    error_build(SN_ERROR_NESTED_FN_EXPR, 1, 5, NULL,
                "(if (fn (foo) null) null 1)\n");

    error_build(SN_ERROR_NESTED_FN_EXPR, 2, 5, NULL,
                "(fn (foo a b)\n"
                "    (fn (nest) null))\n");

    // nor nested lets
    error_build(SN_ERROR_NESTED_LET_EXPR, 1, 8, NULL,
                "(let a (let b 0))\n");

    error_build(SN_ERROR_NESTED_LET_EXPR, 1, 5, NULL,
                "(if (let a 0) null 1)\n");

    // do block - too short
    error_build(SN_ERROR_DO_EXPR_TOO_SHORT, 2, 3, NULL,
                "(fn (main)\n"
                "  (do))\n");
    // ok
    error_build(SN_SUCCESS, 0, 0, NULL,
                "(fn (main)\n"
                "  (do null))\n");
}

sn_value_t *
error_run(sn_error_t err_code,
          int err_line,
          int err_col,
          const char *err_sym,
          const char *src)
{
    sn_value_t *value = sn_value_create();
    sn_program_t *prog = NULL;
    ASSERT_OK(sn_program_create(&prog, src, strlen(src)));
    ASSERT_OK(sn_program_build(prog));
    sn_error_t status = sn_program_run(prog, value);
    if (status != err_code) {
        fprintf(stderr, "sn_program_run returned %s\n", sn_error_str(status));
        abort();
    }
    if (err_code != SN_SUCCESS) {
        error_check(prog, err_line, err_col, err_sym);
    }
    sn_program_destroy(prog);
    return value;
}

void test_run_error()
{
    // wrong number of args to function
    error_run(SN_ERROR_WRONG_ARG_COUNT_IN_CALL, 2, 1, NULL,
              "(fn (foo a) null)\n"
              "(foo)\n");

    // wrong number of args to function
    error_run(SN_ERROR_WRONG_ARG_COUNT_IN_CALL, 2, 1, NULL,
              "(fn (foo) null)\n"
              "(foo 1)\n");

}

sn_value_t *check_run(const char *str)
{
    return error_run(SN_SUCCESS, 0, 0, NULL, str);
}

void test_run_func()
{
    // nested functions
    sn_value_t *val = check_run(
        "(let three 3)\n"
        "(fn (double x)\n"
        "  (+ x x))\n"
        "(fn (triple x)\n"
        "  (+ (double x) x))\n"
        "(triple three)\n");

    ASSERT_EQ(ival(val), 9);

    // local scope is separate
    val = check_run(
        "(let x 3)\n"
        "(fn (double x)\n"
        "  (+ x x))\n"
        "(double x)\n");

    ASSERT_EQ(ival(val), 6);

    // local let
    val = check_run(
        "(let z 3)\n"
        "(fn (inc x)\n"
        "  (let z 2)\n"
        "  (let y -1)\n"
        "  (+ x y z))\n"
        "(inc z)\n");
    ASSERT_EQ(ival(val), 4);

    val = check_run("true");
    ASSERT_EQ(bval(val), true);

    val = check_run("false");
    ASSERT_EQ(bval(val), false);

}

void test_equals(void)
{
    sn_value_t *val = NULL;
    val = check_run("(fn (main)\n"
                    "  {1 == 2})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a 12)\n"
                    "  (let b 12)\n"
                    "  {a == b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a +)\n"
                    "  (let b +)\n"
                    "  {a == b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a +)\n"
                    "  (let b -)\n"
                    "  {a == b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a main)\n"
                    "  (let b main)\n"
                    "  {a == b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a main)\n"
                    "  (let b +)\n"
                    "  {a == b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a ==)\n"
                    "  {a a a})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    error_run(SN_ERROR_WRONG_VALUE_TYPE, 2, 3, NULL,
              "(fn (main)\n"
              "  {0 == false})\n"
              "(main)\n");

    val = check_run("(fn (main)\n"
                    "  {1 != 2})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a 12)\n"
                    "  (let b 12)\n"
                    "  {a != b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a +)\n"
                    "  (let b +)\n"
                    "  {a != b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a +)\n"
                    "  (let b -)\n"
                    "  {a != b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a main)\n"
                    "  (let b main)\n"
                    "  {a != b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (let a main)\n"
                    "  (let b +)\n"
                    "  {a != b})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a !=)\n"
                    "  {a a a})\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    error_run(SN_ERROR_WRONG_VALUE_TYPE, 2, 3, NULL,
              "(fn (main)\n"
              "  {0 != false})\n"
              "(main)\n");

    val = check_run("(fn (main)\n"
                    "  (! {1 == 2}))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (let a 12)\n"
                    "  (let b 12)\n"
                    "  (! {a == b}))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    error_run(SN_ERROR_WRONG_VALUE_TYPE, 2, 3, NULL,
              "(fn (main)\n"
              "  (! null))\n"
              "(main)\n");
}

void test_type_queries(void)
{
    sn_value_t *val = NULL;
    val = check_run("(fn (main)\n"
                    "  (null? null))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (null? +))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (fn? fn?))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (fn? main))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (fn? true))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);

    val = check_run("(fn (main)\n"
                    "  (int? 1234))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), true);

    val = check_run("(fn (main)\n"
                    "  (int? fn?))\n"
                    "(main)\n");
    ASSERT_EQ(bval(val), false);
}

void test_if(void)
{
    sn_value_t *val = NULL;
    val = check_run("(fn (main)\n"
                    "  (if (fn? main) 10 20))\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 10);

    val = check_run("(fn (main)\n"
                    "  (if (int? main) 10 20))\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 20);
}

void test_math(void)
{
    sn_value_t *val = NULL;
    val = check_run("(fn (main)\n"
                    "  (* 2 3 4))\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 24);

    val = check_run("(fn (main)\n"
                    "  (/ 100 20))\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 5);

    val = check_run("(fn (main)\n"
                    "  (% 19 8))\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 3);
}

void test_factorial(void)
{
    sn_value_t *val = NULL;
    val = check_run("(fn (fact n)\n"
                    "  (if {n == 0}\n"
                    "     1\n"
                    "     {n * (fact {n - 1})}))\n"
                    "(fact 5)\n");
    ASSERT_EQ(ival(val), 120);

    val = check_run("(fn (fact n)\n"
                    "  (if {n == 0}\n"
                    "     1\n"
                    "     {n * (fact {n - 1})}))\n"
                    "(fact 10)\n");
    ASSERT_EQ(ival(val), 3628800);
}

void test_do(void)
{
    error_build(SN_ERROR_UNDECLARED, 4, 4, "x",
                "(fn (main)\n"
                "  (do\n"
                "    (let x 10))\n"
                "  {x + 1})\n");

    error_build(SN_ERROR_REDECLARED, 2, 12, "arg",
                "(fn (main arg)\n"
                "  (do (let arg 0)))\n");

    sn_value_t *val = NULL;
    val = check_run("(fn (add10 x)\n"
                    "  (do\n"
                    "    (let y 10)\n"
                    "    {x + y}))\n"
                    "(add10 5)\n");
    ASSERT_EQ(ival(val), 15);

    val = check_run("(fn (add20 x)\n"
                    "  (do\n"
                    "    (let y 5)\n"
                    "    (+ (do\n"
                    "         (let z 15)\n"
                    "         {x + z})\n"
                    "       y)))\n"
                    "(add20 10)\n");
    ASSERT_EQ(ival(val), 30);

    val = check_run("(fn (add20 x)\n"
                    "  (do\n"
                    "    (let y 5)\n"
                    "    (+ (do\n"
                    "         (let z 15)\n"
                    "         (+ x y z)))))\n"
                    "(add20 10)\n");
    ASSERT_EQ(ival(val), 30);

    val = check_run("(fn (times5 x)\n"
                    "  (+ (do\n"
                    "       (let x2 {x + x})\n"
                    "       x2)\n"
                    "     (do\n"
                    "       (let x3 (+ x x x))\n"
                    "       x3)))\n"
                    "(times5 5)\n");
    ASSERT_EQ(ival(val), 25);
}

void test_assign(void)
{
    error_build(SN_ERROR_UNDECLARED, 2, 4, "x",
                "(fn (main)\n"
                "  {x = 10})\n");

    error_build(SN_ERROR_EXPR_BAD_DEST, 2, 4, NULL,
                "(fn (main)\n"
                "  {(+ 1 2) = 10})\n");


    sn_value_t *val = NULL;
    val = check_run("(fn (main)\n"
                    "  (let x 1)\n"
                    "  {x = {x + 1}}\n"
                    "  x)\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 2);

    val = check_run("(fn (main)\n"
                    "  (let x 1)\n"
                    "  {x = {x + 1}}\n"
                    "  x)\n"
                    "(main)\n");
    ASSERT_EQ(ival(val), 2);
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
    test_run_error();
    test_run_func();
    test_equals();
    test_type_queries();
    test_if();
    test_math();
    test_factorial();
    test_do();
    test_assign();
    printf("PASSED\n");
    return 0;
}
