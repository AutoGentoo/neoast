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
} ValUnion;

int ll_skip(const char* yytext, void* yyval, char* skip)
{
    (void)yytext;
    (void)yyval;

    *skip = 1;
    return 0;
}
int ll_tok_num(const char* yytext, ValUnion* yyval, const char* skip)
{
    (void)skip;

    yyval->integer = (int)strtol(yytext, NULL, 10);
    return TOK_NUMBER;
}

int gg_expr_seq_1(ValUnion* dest, ValUnion* args)
{
    dest->expr = malloc(sizeof(ValUnion));

    dest->expr->val = args[0].integer;
    dest->expr->next = NULL;

    return TOK_EXPR;
}

int gg_expr_seq_2(ValUnion* dest, ValUnion* args)
{
    dest->expr = args[0].expr;
    dest->expr->next = args[1].expr;
    return TOK_EXPR;
}

static GrammarParser p;

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.expr = ll_skip, .regex = 0},
            {.expr = (lexer_expr) ll_tok_num, .regex = 0}
    };

    static int tok_seq_1[] = {
            TOK_NUMBER,
            TOK_TERM
    };

    static int tok_seq_2[] = {
            TOK_EXPR,
            TOK_EXPR,
            TOK_TERM
    };

    static GrammarRule g_rules[] = {
            {.grammar = tok_seq_1, .expr = (parser_expr) gg_expr_seq_1},
            {.grammar = tok_seq_2, .expr = (parser_expr) gg_expr_seq_2}
    };

    p.grammar_n = 2;
    p.grammar_rules = g_rules;
    p.lex_n = 2;
    p.lexer_rules = l_rules;

    // Initialize the lexer regex rules
    assert_int_equal(regcomp(&l_rules[0].regex, "^[ ]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[1].regex, "^[0-9]+", REG_EXTENDED), 0);
}

CTEST(test_lexer)
{
    const char* lexer_input = "10 20 30";
    initialize_parser();

    const char** yyinput = &lexer_input;

    ValUnion llval;

    int tok;
    while((tok = lex_next(yyinput, &p, &llval)))
    {
        printf("tok: %d %d\n", tok, llval.integer);
    }
}

CTEST(test_parser)
{
    const char* lexer_input = "10 20 30";
    initialize_parser();

    const char** yyinput = &lexer_input;

    int token_table[32];
    ValUnion value_table[32];

    int tok_n = parser_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 3);

    parser_parse(&p, &tok_n, token_table, value_table, sizeof(ValUnion));

    assert_int_equal(tok_n, 1);

    assert_int_equal(token_table[0], TOK_EXPR);
    assert_non_null(value_table[0].expr);
    assert_int_equal(value_table[0].expr->val, 10);

    assert_non_null(value_table[0].expr->next);
    assert_int_equal(value_table[0].expr->next->val, 20);

    assert_non_null(value_table[0].expr->next->next);
    assert_int_equal(value_table[0].expr->next->next->val, 30);
}

const static struct CMUnitTest hacksaw_tests[] = {
        cmocka_unit_test(test_regexec),
        cmocka_unit_test(test_lexer),
        cmocka_unit_test(test_parser),
};

int main()
{
    return cmocka_run_group_tests(hacksaw_tests, NULL, NULL);
}