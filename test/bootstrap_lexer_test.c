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
#include <string.h>
#include <stdlib.h>
#include <codegen/bootstrap/lexer/bootstrap_lexer.h>

#define CTEST(name) static void name(void** state)

typedef union {
    int integer;
    struct expr_ {
        int val;
        struct expr_* next;
    }* expr;
} CodegenUnion;

int32_t ll_tok_num(const char* yytext, CodegenUnion* yyval)
{
    yyval->integer = (int)strtol(yytext, NULL, 10);
    return NEOAST_ASCII_MAX + 1;
}

static GrammarParser p;
static void* l;

void initialize_parser()
{
    static const LexerRule l_rules[] = {
            {.regex = "[ ]+", .tok = -1},
            {.expr = (lexer_expr) ll_tok_num, .regex = "[0-9]+"}

    };

    static const LexerRule* ll_rules[] = {
            l_rules
    };

    p.grammar_n = 0;
    p.grammar_rules = 0;

    static uint32_t lex_n = NEOAST_ARR_LEN(l_rules);
    l = bootstrap_lexer_new(ll_rules, &lex_n, 1, NULL, sizeof(CodegenUnion), NULL);
}

CTEST(test_lexer)
{
    const char* lexer_input = "10 20 30 a";
    initialize_parser();

    const char* yyinput = lexer_input;

    struct {
        CodegenUnion value;
        TokenPosition position;
    } llval;

    ParserBuffers* buf = parser_allocate_buffers(256, 256, sizeof(CodegenUnion), sizeof(CodegenUnion));
    void* lex = bootstrap_lexer_instance_new(l, yyinput, strlen(lexer_input));

    assert_int_equal(bootstrap_lexer_next(lex, &llval), 1);
    assert_int_equal(llval.value.integer, 10);
    assert_int_equal(bootstrap_lexer_next(lex, &llval), 1);
    assert_int_equal(llval.value.integer, 20);
    assert_int_equal(bootstrap_lexer_next(lex, &llval), 1);
    assert_int_equal(llval.value.integer, 30);
    assert_int_equal(bootstrap_lexer_next(lex, &llval), -1); // Unhandled 'a'
    assert_int_equal(bootstrap_lexer_next(lex, &llval), -1);

    bootstrap_lexer_instance_free(lex);
    parser_free_buffers(buf);
    bootstrap_lexer_free(l);
}

const static struct CMUnitTest lexer_tests[] = {
        cmocka_unit_test(test_lexer),
};

int main()
{
    return cmocka_run_group_tests(lexer_tests, NULL, NULL);
}
