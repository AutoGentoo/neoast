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
#include <stddef.h>
#include <lexer/matcher.h>
#include <lexer/matcher_fsm.h>
#include <lexer/input.h>

#define CTEST(name) static void name(void** state)

void pattern_fsm(NeoastMatcher* m)
{
    int c0, c1 = 0;
    FSM_INIT(m, &c1);

    S0:
    FSM_FIND(m);
    c1 = FSM_CHAR(m);
    if ('a' <= c1 && c1 <= 'z') goto S12;
    if (c1 == '_') goto S12;
    if ('A' <= c1 && c1 <= 'Z') goto S12;
    if (c1 == '=') goto S27;
    if ('0' <= c1 && c1 <= '9') goto S18;
    if (c1 == '.') goto S24;
    if ('-' <= c1 && c1 <= '/') goto S27;
    if ('(' <= c1 && c1 <= '+') goto S27;
    if (c1 == ' ') goto S35;
    if (c1 == '\n') goto S29;
    if ('\t' <= c1 && c1 <= '\r') goto S35;
    if (0 <= c1) goto S33;
    return FSM_HALT(m, c1);

    S12:
    FSM_TAKE(m, 1, EOF);
    c1 = FSM_CHAR(m);
    if ('a' <= c1 && c1 <= 'z') goto S12;
    if (c1 == '_') goto S12;
    if ('A' <= c1 && c1 <= 'Z') goto S12;
    if ('0' <= c1 && c1 <= '9') goto S12;
    return FSM_HALT(m, c1);

    S18:
    FSM_TAKE(m, 2, EOF);
    c1 = FSM_CHAR(m);
    if (c1 == 'e') goto S44;
    if (c1 == 'E') goto S44;
    if ('0' <= c1 && c1 <= '9') goto S18;
    if (c1 == '.') goto S39;
    return FSM_HALT(m, c1);

    S24:
    FSM_TAKE(m, 6, EOF);
    c1 = FSM_CHAR(m);
    if ('0' <= c1 && c1 <= '9') goto S48;
    return FSM_HALT(m, c1);

    S27:
    FSM_TAKE(m, 3, EOF);
    return FSM_HALT(m, CONST_UNK);

    S29:
    FSM_TAKE(m, 4, EOF);
    c1 = FSM_CHAR(m);
    if (c1 == ' ') goto S35;
    if ('\t' <= c1 && c1 <= '\r') goto S35;
    return FSM_HALT(m, c1);

    S33:
    FSM_TAKE(m, 6, EOF);
    return FSM_HALT(m, CONST_UNK);

    S35:
    FSM_TAKE(m, 5, EOF);
    c1 = FSM_CHAR(m);
    if (c1 == ' ') goto S35;
    if ('\t' <= c1 && c1 <= '\r') goto S35;
    return FSM_HALT(m, c1);

    S39:
    FSM_TAKE(m, 2, EOF);
    c1 = FSM_CHAR(m);
    if (c1 == 'e') goto S44;
    if (c1 == 'E') goto S44;
    if ('0' <= c1 && c1 <= '9') goto S48;
    return FSM_HALT(m, c1);

    S44:
    c1 = FSM_CHAR(m);
    if ('0' <= c1 && c1 <= '9') goto S55;
    if (c1 == '-') goto S53;
    if (c1 == '+') goto S53;
    return FSM_HALT(m, c1);

    S48:
    FSM_TAKE(m, 2, EOF);
    c1 = FSM_CHAR(m);
    if (c1 == 'e') goto S44;
    if (c1 == 'E') goto S44;
    if ('0' <= c1 && c1 <= '9') goto S48;
    return FSM_HALT(m, c1);

    S53:
    c1 = FSM_CHAR(m);
    if ('0' <= c1 && c1 <= '9') goto S55;
    return FSM_HALT(m, c1);

    S55:
    FSM_TAKE(m, 2, EOF);
    c1 = FSM_CHAR(m);
    if ('0' <= c1 && c1 <= '9') goto S55;
    return FSM_HALT(m, c1);
}


CTEST(test_lexer)
{
    static const char test_string[] = "123     + variable\n";

    NeoastInput* input = input_new_from_buffer(test_string, sizeof(test_string) - 1);
    assert_non_null(input);

    NeoastMatcher* mat = matcher_new(input);
    assert_non_null(mat);

    typedef enum {
        TOK_EOF,
        TOK_VARIABLE,
        TOK_NUMBER,
        TOK_OPERATOR,
        TOK_LF,
        TOK_WS,
        TOK_UNK
    } token_ids;

    size_t tok;

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_NUMBER);
    assert_string_equal(matcher_text(mat), "123");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_WS);
    assert_string_equal(matcher_text(mat), "     ");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_OPERATOR);
    assert_string_equal(matcher_text(mat), "+");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_WS);
    assert_string_equal(matcher_text(mat), " ");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_VARIABLE);
    assert_string_equal(matcher_text(mat), "variable");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_LF);
    assert_string_equal(matcher_text(mat), "\n");

    tok = matcher_scan(mat, pattern_fsm);
    assert_int_equal(tok, TOK_EOF);

    input_free(input);
    matcher_free(mat);
}

const static struct CMUnitTest neoast_lexer_tests[] = {
        cmocka_unit_test(test_lexer),
};

int main()
{
    return cmocka_run_group_tests(neoast_lexer_tests, NULL, NULL);
}
