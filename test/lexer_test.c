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
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <lexer.h>

#define CTEST(name) static void name(void** state)

CTEST(test_regexec)
{
    size_t len;
    char result[BUFSIZ];
    int i;
    regmatch_t p[3];
    p[0].rm_so = 1;
    p[0].rm_eo = 4;

    const char src[] = " test33 string 123";

    regex_t c;
    assert_int_equal(regcomp(&c, "[A-z]+", REG_EXTENDED), 0);
    assert_int_equal(regexec(&c, src, 2, p, 0), 0);

    for (i = 0; p[i].rm_so != -1; i++)
    {
        len = p[i].rm_eo - p[i].rm_so;
        memcpy(result, src + p[i].rm_so, len);
        result[len] = 0;
        assert_string_equal(result, "test");
    }

    assert_int_equal(i, 1);
    regfree(&c);
}

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
    return 1;
}

static GrammarParser p;

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.regex_raw = "[ ]+", .tok = -1},
            {.expr = (lexer_expr) ll_tok_num, .regex_raw = "[0-9]+"}

    };

    static LexerRule* ll_rules[] = {
            l_rules
    };

    p.grammar_n = 0;
    p.grammar_rules = 0;

    static uint32_t lex_n = NEOAST_ARR_LEN(l_rules);

    p.lex_state_n = 1;
    p.lex_n = &lex_n;
    p.lexer_rules = ll_rules;

    // Initialize the lexer regex rules
    parser_init(&p);
}

CTEST(test_lexer)
{
    const char* lexer_input = "10 20 30 a";
    initialize_parser();

    const char* yyinput = lexer_input;

    CodegenUnion llval;

    ParserBuffers* buf = parser_allocate_buffers(32, 32, 8, sizeof(CodegenUnion));
    NEOAST_STACK_PUSH(buf->lexing_state_stack, 0);

    uint32_t len = strlen(lexer_input);
    uint32_t offset = 0;
    assert_int_equal(lex_next(yyinput, &p, buf, &llval, len, &offset), 1);
    assert_int_equal(llval.integer, 10);
    assert_int_equal(lex_next(yyinput, &p, buf, &llval, len, &offset), 1);
    assert_int_equal(llval.integer, 20);
    assert_int_equal(lex_next(yyinput, &p, buf, &llval, len, &offset), 1);
    assert_int_equal(llval.integer, 30);
    assert_int_equal(lex_next(yyinput, &p, buf, &llval, len, &offset), -1); // Unhandled 'a'
    assert_int_equal(lex_next(yyinput, &p, buf, &llval, len, &offset), 0);

    parser_free(&p);
    parser_free_buffers(buf);
}

const static struct CMUnitTest lexer_tests[] = {
        cmocka_unit_test(test_regexec),
        cmocka_unit_test(test_lexer),
};

int main()
{
    return cmocka_run_group_tests(lexer_tests, NULL, NULL);
}
