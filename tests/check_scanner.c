#include "../src/scanner.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

START_TEST(test_scanner_single_char_token) {
    initScanner("( ) { } , . - + / * \n");
    ck_assert(scanToken().type == TOKEN_LEFT_PAREN);
    ck_assert(scanToken().type == TOKEN_RIGHT_PAREN);
    ck_assert(scanToken().type == TOKEN_LEFT_BRACE);
    ck_assert(scanToken().type == TOKEN_RIGHT_BRACE);
    ck_assert(scanToken().type == TOKEN_COMMA);
    ck_assert(scanToken().type == TOKEN_DOT);
    ck_assert(scanToken().type == TOKEN_MINUS);
    ck_assert(scanToken().type == TOKEN_PLUS);
    ck_assert(scanToken().type == TOKEN_SLASH);
    ck_assert(scanToken().type == TOKEN_STAR);
    ck_assert(scanToken().type == TOKEN_NEWLINE);
}
END_TEST

START_TEST(test_scanner_one_or_two_char_token) {
    initScanner("! != = == > >= < <= & && | ||");
    ck_assert(scanToken().type == TOKEN_BANG);
    ck_assert(scanToken().type == TOKEN_BANG_EQUAL);
    ck_assert(scanToken().type == TOKEN_EQUAL);
    ck_assert(scanToken().type == TOKEN_EQUAL_EQUAL);
    ck_assert(scanToken().type == TOKEN_GREATER);
    ck_assert(scanToken().type == TOKEN_GREATER_EQUAL);
    ck_assert(scanToken().type == TOKEN_LESS);
    ck_assert(scanToken().type == TOKEN_LESS_EQUAL);
    ck_assert(scanToken().type == TOKEN_BITAND);
    ck_assert(scanToken().type == TOKEN_AND);
    ck_assert(scanToken().type == TOKEN_BITOR);
    ck_assert(scanToken().type == TOKEN_OR);
}
END_TEST

START_TEST(test_scanner_literal) {
    initScanner("a \"aaa\" `this is a multi\nline\n string` 123");
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_STRING);
    ck_assert(scanToken().type == TOKEN_STRING);
    ck_assert(scanToken().type == TOKEN_NUMBER);
}
END_TEST

START_TEST(test_scanner_correct_keywords) {
    initScanner("var true false nil print if else while for break continue "
                "this class super fn return");

    ck_assert(scanToken().type == TOKEN_VAR);
    ck_assert(scanToken().type == TOKEN_TRUE);
    ck_assert(scanToken().type == TOKEN_FALSE);
    ck_assert(scanToken().type == TOKEN_NIL);
    ck_assert(scanToken().type == TOKEN_PRINT);
    ck_assert(scanToken().type == TOKEN_IF);
    ck_assert(scanToken().type == TOKEN_ELSE);
    ck_assert(scanToken().type == TOKEN_WHILE);
    ck_assert(scanToken().type == TOKEN_FOR);
    ck_assert(scanToken().type == TOKEN_BREAK);
    ck_assert(scanToken().type == TOKEN_CONTINUE);
    ck_assert(scanToken().type == TOKEN_THIS);
    ck_assert(scanToken().type == TOKEN_CLASS);
    ck_assert(scanToken().type == TOKEN_SUPER);
    ck_assert(scanToken().type == TOKEN_FN);
    ck_assert(scanToken().type == TOKEN_RETURN);
}
END_TEST

START_TEST(test_scanner_mixcase_keywords) {
    initScanner("Var True False Nil Print iF eLse whIle For bReak Continue "
                "This Class sUper Fn Return");

    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
    ck_assert(scanToken().type == TOKEN_IDENTIFIER);
}
END_TEST

START_TEST(test_scanner_valid_skipwhitespace) {
    initScanner(
        "// this is a good test \n /* this \n should be skipped */ \t\r this ");
    ck_assert(scanToken().type == TOKEN_THIS);
}
END_TEST

START_TEST(test_scanner_invalid_skipwhitespace) {
    initScanner("/* /* this \n should be skipped */");
    Token token = scanToken();
    ck_assert(token.type == TOKEN_ERROR);
    ck_assert_str_eq("Unending block comment", token.start);
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