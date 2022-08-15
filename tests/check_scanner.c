#include "../src/scanner.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void assertOnlyError() {
    Token *token = nextToken();
    while (token->type != TOKEN_EOF) {
        ck_assert(token->type == TOKEN_ERROR);
    }
}

START_TEST(test_scanner_single_char_token) {
    initScanner("( ) { } , . - + / * \n");
    ck_assert(nextToken()->type == TOKEN_LEFT_PAREN);
    ck_assert(nextToken()->type == TOKEN_RIGHT_PAREN);
    ck_assert(nextToken()->type == TOKEN_LEFT_BRACE);
    ck_assert(nextToken()->type == TOKEN_RIGHT_BRACE);
    ck_assert(nextToken()->type == TOKEN_COMMA);
    ck_assert(nextToken()->type == TOKEN_DOT);
    ck_assert(nextToken()->type == TOKEN_MINUS);
    ck_assert(nextToken()->type == TOKEN_PLUS);
    ck_assert(nextToken()->type == TOKEN_SLASH);
    ck_assert(nextToken()->type == TOKEN_STAR);
    ck_assert(nextToken()->type == TOKEN_NEWLINE);
}
END_TEST

START_TEST(test_scanner_one_or_two_char_token) {
    initScanner("? : ! != = == > >= < <= && ||");
    ck_assert(nextToken()->type == TOKEN_QUESTION);
    ck_assert(nextToken()->type == TOKEN_COLON);
    ck_assert(nextToken()->type == TOKEN_BANG);
    ck_assert(nextToken()->type == TOKEN_BANG_EQUAL);
    ck_assert(nextToken()->type == TOKEN_EQUAL);
    ck_assert(nextToken()->type == TOKEN_EQUAL_EQUAL);
    ck_assert(nextToken()->type == TOKEN_GREATER);
    ck_assert(nextToken()->type == TOKEN_GREATER_EQUAL);
    ck_assert(nextToken()->type == TOKEN_LESS);
    ck_assert(nextToken()->type == TOKEN_LESS_EQUAL);
    ck_assert(nextToken()->type == TOKEN_AND);
    ck_assert(nextToken()->type == TOKEN_OR);
}
END_TEST

START_TEST(test_scanner_literal) {
    initScanner("a \"aaa\" `this is a multi\nline\n string` 123");
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_STRING);
    ck_assert(nextToken()->type == TOKEN_STRING);
    ck_assert(nextToken()->type == TOKEN_NUMBER);
}
END_TEST

START_TEST(test_scanner_correct_keywords) {
    initScanner("var true false nil print if else while for break continue "
                "this class super fn return");

    ck_assert(nextToken()->type == TOKEN_VAR);
    ck_assert(nextToken()->type == TOKEN_TRUE);
    ck_assert(nextToken()->type == TOKEN_FALSE);
    ck_assert(nextToken()->type == TOKEN_NIL);
    ck_assert(nextToken()->type == TOKEN_PRINT);
    ck_assert(nextToken()->type == TOKEN_IF);
    ck_assert(nextToken()->type == TOKEN_ELSE);
    ck_assert(nextToken()->type == TOKEN_WHILE);
    ck_assert(nextToken()->type == TOKEN_FOR);
    ck_assert(nextToken()->type == TOKEN_BREAK);
    ck_assert(nextToken()->type == TOKEN_CONTINUE);
    ck_assert(nextToken()->type == TOKEN_THIS);
    ck_assert(nextToken()->type == TOKEN_CLASS);
    ck_assert(nextToken()->type == TOKEN_SUPER);
    ck_assert(nextToken()->type == TOKEN_FN);
    ck_assert(nextToken()->type == TOKEN_RETURN);
}
END_TEST

START_TEST(test_scanner_mixcase_keywords) {
    initScanner("Var True False Nil Print iF eLse whIle For bReak Continue "
                "This Class sUper Fn Return");

    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
    ck_assert(nextToken()->type == TOKEN_IDENTIFIER);
}
END_TEST

START_TEST(test_scanner_valid_skipwhitespace) {
    initScanner(
        "// this is a good test \n /* this \n should be skipped */ \t\r this ");
    ck_assert(nextToken()->type == TOKEN_THIS);
}
END_TEST

START_TEST(test_scanner_invalid_skipwhitespace) {
    initScanner("/* /* this \n should be skipped */");
    Token *token = nextToken();
    ck_assert(token->type == TOKEN_ERROR);
    ck_assert_str_eq("Unending block comment", token->start);
}
END_TEST

START_TEST(test_scanner_string_template) {
    initScanner("`Has head ${false} Has mid ${true} Has tail`");
    Token *head = nextToken();
    ck_assert(head->type == TOKEN_PRE_TEMPLATE);
    ck_assert(memcmp("`Has head ", head->start, sizeof(char) * 10) == 0);
    ck_assert(nextToken()->type == TOKEN_FALSE);
    Token *tween = nextToken();
    ck_assert(tween->type == TOKEN_TWEEN_TEMPLATE);
    ck_assert(memcmp(" Has mid ", tween->start, sizeof(char) * 9) == 0);
    ck_assert(nextToken()->type == TOKEN_TRUE);
    Token *after = nextToken();
    ck_assert(after->type == TOKEN_AFTER_TEMPLATE);
    ck_assert(memcmp(" Has tail`", after->start, sizeof(char) * 10) == 0);
    ck_assert(nextToken()->type == TOKEN_EOF);
}
END_TEST

START_TEST(test_scanner_nested_string_template) {
    initScanner("`Has head ${`${false}`} Has tail`");
    ck_assert(nextToken()->type == TOKEN_PRE_TEMPLATE);
    ck_assert(nextToken()->type == TOKEN_PRE_TEMPLATE);
    ck_assert(nextToken()->type == TOKEN_FALSE);
    ck_assert(nextToken()->type == TOKEN_AFTER_TEMPLATE);
    ck_assert(nextToken()->type == TOKEN_AFTER_TEMPLATE);
}
END_TEST

START_TEST(test_scanner_too_many_nested_string_template) {
    initScanner("`${`${`${`${`${`${false}`}`}`}`}`}`");
    assertOnlyError();
}
END_TEST

START_TEST(test_scanner_incomplete_string_template) {
    initScanner("`${}");
    assertOnlyError();
    initScanner("`${`");
    assertOnlyError();
    initScanner("`${`${`}`");
    assertOnlyError();
}
END_TEST

START_TEST(test_scanner_error_in_template) {
    initScanner("`${`${false ^ true}`}`");
    assertOnlyError();
}
END_TEST
Suite *scanner_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Scanner");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_scanner_single_char_token);
    tcase_add_test(tc_core, test_scanner_one_or_two_char_token);
    tcase_add_test(tc_core, test_scanner_literal);
    tcase_add_test(tc_core, test_scanner_correct_keywords);
    tcase_add_test(tc_core, test_scanner_mixcase_keywords);
    tcase_add_test(tc_core, test_scanner_valid_skipwhitespace);
    tcase_add_test(tc_core, test_scanner_invalid_skipwhitespace);
    tcase_add_test(tc_core, test_scanner_string_template);
    tcase_add_test(tc_core, test_scanner_nested_string_template);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int no_failed = 0;
    Suite *s;
    SRunner *runner;
    s = scanner_suite();
    runner = srunner_create(s);

    srunner_run_all(runner, CK_NORMAL);
    no_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}