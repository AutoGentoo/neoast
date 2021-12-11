/* 
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <cmocka.h>
#include <neoast.h>

#define CTEST(name) static void name(void** state)

#define FUNC(name, suffix) name ## _ ## suffix

#define DEFINE_HEADER(name, return_type) \
uint32_t FUNC(name, init)(); \
void* FUNC(name, allocate_buffers)(); \
void FUNC(name, free_buffers)(void* self); \
void FUNC(name, free)(); \
return_type FUNC(name, parse)(const void* buffers, const char* input);

// Pretend headers
DEFINE_HEADER(calc, double)
DEFINE_HEADER(calc_ascii, double)
DEFINE_HEADER(required_use, void*)
DEFINE_HEADER(error, int)

void required_use_stmt_free(void* self);

CTEST(test_empty)
{
    assert_int_equal(calc_init(), 0);
    void* buffers = calc_allocate_buffers();

    assert_double_equal(calc_parse(buffers, "   "), 0, 0);

    calc_free();
    calc_free_buffers(buffers);
}

CTEST(test_parser)
{
    assert_int_equal(calc_init(), 0);
    void* buffers = calc_allocate_buffers();

    const char* input = "3 + 5 + (4 * 2 + (5 / 2))";
    double result = calc_parse(buffers, input);
    assert_double_equal(result, 3 + 5 + (4 * 2 + (5.0 / 2)), 0.001);
    calc_free_buffers(buffers);
    calc_free();
}

CTEST(test_empty_ascii)
{
    assert_int_equal(calc_ascii_init(), 0);
    void* buffers = calc_ascii_allocate_buffers();

    assert_double_equal(calc_ascii_parse(buffers, "   "), 0, 0);

    calc_ascii_free();
    calc_ascii_free_buffers(buffers);
}

CTEST(test_parser_ascii)
{
    assert_int_equal(calc_ascii_init(), 0);
    void* buffers = calc_ascii_allocate_buffers();

    const char* input = "3 + 5 + (4 * 2 + (5 / 2))";
    double result = calc_ascii_parse(buffers, input);
    assert_double_equal(result, 3 + 5 + (4 * 2 + (5.0 / 2)), 0.001);
    calc_ascii_free_buffers(buffers);
    calc_ascii_free();
}

CTEST(test_parser_ascii_order_of_ops)
{
    assert_int_equal(calc_ascii_init(), 0);
    void* buffers = calc_ascii_allocate_buffers();

    const char* input = "3 + 5 * 5 + 3 * 6 + 3";
    double result = calc_ascii_parse(buffers, input);
    assert_double_equal(result, 3 + 5 * 5 + 3 * 6 + 3, 0.001);
    calc_ascii_free_buffers(buffers);
    calc_ascii_free();
}

CTEST(test_destructor)
{
    assert_int_equal(required_use_init(), 0);
    void* buffers = required_use_allocate_buffers();
    void* stmt = required_use_parse(buffers, "?? ( hello_world");
    assert_null(stmt);
    required_use_stmt_free(stmt);
    required_use_free_buffers(buffers);
    required_use_free();
}

CTEST(test_destructor_lex)
{
    assert_int_equal(required_use_init(), 0);
    void* buffers = required_use_allocate_buffers();
    void* stmt = required_use_parse(buffers, "( hello_world %%%% )");
    assert_null(stmt);
    required_use_stmt_free(stmt);
    required_use_free_buffers(buffers);
    required_use_free();
}

static volatile int lexer_error_called = 0;
static volatile int parser_error_called = 0;

void lexer_error_cb(const char* input,
                    const TokenPosition* position,
                    const char* lexer_state)
{
    (void) input;
    assert_int_equal(position->line, 2);
    assert_int_equal(position->col_start, 4);
    assert_string_equal(lexer_state, "LEX_STATE_DEFAULT");
    lexer_error_called = 1;
}

void parser_error_cb(const char* const* token_names,
                     const TokenPosition* position,
                     uint32_t last_token,
                     uint32_t current_token,
                     const uint32_t expected_tokens[],
                     uint32_t expected_tokens_n)
{
    assert_non_null(token_names);
    assert_non_null(position);
    assert_non_null(expected_tokens);

    assert_int_equal(last_token, 1);
    assert_int_equal(current_token, 1);

    assert_string_equal(token_names[last_token], "A_TOKEN");
    assert_int_equal(expected_tokens_n, 5); /* 4 operations and EOF */

    assert_string_equal(token_names[current_token], "A_TOKEN");

    assert_string_equal(token_names[expected_tokens[0]], "EOF");
    assert_string_equal(token_names[expected_tokens[1]], "/");
    assert_string_equal(token_names[expected_tokens[2]], "*");
    assert_string_equal(token_names[expected_tokens[3]], "-");
    assert_string_equal(token_names[expected_tokens[4]], "+");

    assert_int_equal(position->line, 1);
    assert_int_equal(position->col_start, 13);

    parser_error_called = 1;
}

CTEST(test_error_ll)
{
    lexer_error_called = 0;
    assert_int_equal(error_init(), 0);
    void* buffers = error_allocate_buffers();
    int out = error_parse(buffers, "\n5555;");
    assert_int_equal(out, 0);
    assert_int_equal(lexer_error_called, 1);
    error_free();
    error_free_buffers(buffers);
}

CTEST(test_error_yy)
{
    parser_error_called = 0;
    assert_int_equal(error_init(), 0);
    void* buffers = error_allocate_buffers();
    int out = error_parse(buffers, "55 + 55 + 99 99");
    assert_int_equal(parser_error_called, 1);
    assert_int_equal(out, 0);
    error_free();
    error_free_buffers(buffers);
}

const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_parser),
        cmocka_unit_test(test_empty_ascii),
        cmocka_unit_test(test_parser_ascii),
        cmocka_unit_test(test_destructor),
        cmocka_unit_test(test_destructor_lex),
        cmocka_unit_test(test_error_ll),
        cmocka_unit_test(test_error_yy),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}
