//
// Created by tumbar on 12/22/20.
//

#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <lexer.h>
#include <parser.h>

#define CTEST(name) static void name(void** state)

CTEST(test_regexec)
{
    int len;
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

int ll_skip(const char* yytext, void* yyval, char* skip) {*skip = 1; return 0;}
int ll_tok_num(const char* yytext, void* yyval, char* skip)
{
    *((int*)yyval) = (int)strtol(yytext, NULL, 10);
    return 1;
}

CTEST(test_lexer)
{
    const char* lexer_input = "10 20 30";

    LexerRule l_rules[] = {
            {.expr = ll_skip, .regex = 0},
            {.expr = ll_tok_num, .regex = 0}
    };

    GrammarParser p = {
            .grammar_n = 0,
            .grammar_rules = NULL,
            .lex_n = 2,
            .lexer_rules = l_rules
    };

    assert_int_equal(regcomp(&l_rules[0].regex, "^[ ]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[1].regex, "^[0-9]+", REG_EXTENDED), 0);

    const char** yyinput = &lexer_input;
    union {
        int integer;
    } llval = {.integer = 0};

    int tok;
    while((tok = lex_next(yyinput, &p, &llval)))
    {
        printf("tok: %d %d\n", tok, llval.integer);
    }
}

const static struct CMUnitTest hacksaw_tests[] = {
        cmocka_unit_test(test_regexec),
        cmocka_unit_test(test_lexer),
};

int main()
{
    return cmocka_run_group_tests(hacksaw_tests, NULL, NULL);
}