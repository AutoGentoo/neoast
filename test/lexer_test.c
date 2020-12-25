//
// Created by tumbar on 12/22/20.
//

#include <stdio.h>
#include <cmocka.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <lexer.h>
#include <parser.h>

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

enum {
    TOK_TERM,
    TOK_NUMBER,
    TOK_EXPR
};

typedef union {
    int integer;
    struct expr_ {
        int val;
        struct expr_* next;
    }* expr;
} LexerUnion;

I32 ll_skip(const char* yytext, void* yyval)
{
    (void)yytext;
    (void)yyval;
    return -1;
}
I32 ll_tok_num(const char* yytext, LexerUnion* yyval)
{
    yyval->integer = (int)strtol(yytext, NULL, 10);
    return TOK_NUMBER;
}

static GrammarParser p;

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.expr = ll_skip, .regex_raw = "^[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex_raw = "^[0-9]+"}

    };

    p.grammar_n = 0;
    p.grammar_rules = 0;
    p.lex_n = 2;
    p.lexer_rules = l_rules;

    // Initialize the lexer regex rules
    parser_init(&p);
}

CTEST(test_lexer)
{
    const char* lexer_input = "10 20 30 a";
    initialize_parser();

    const char** yyinput = &lexer_input;

    LexerUnion llval;

    assert_int_equal(lex_next(yyinput, &p, &llval), 1);
    assert_int_equal(llval.integer, 10);
    assert_int_equal(lex_next(yyinput, &p, &llval), 1);
    assert_int_equal(llval.integer, 20);
    assert_int_equal(lex_next(yyinput, &p, &llval), 1);
    assert_int_equal(llval.integer, 30);
    assert_int_equal(lex_next(yyinput, &p, &llval), -1); // Unhandled 'a'
    assert_int_equal(lex_next(yyinput, &p, &llval), 0);

    parser_free(&p);
}

const static struct CMUnitTest lexer_tests[] = {
        cmocka_unit_test(test_regexec),
        cmocka_unit_test(test_lexer),
};

int main()
{
    return cmocka_run_group_tests(lexer_tests, NULL, NULL);
}