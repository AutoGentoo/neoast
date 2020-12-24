//
// Created by tumbar on 12/23/20.
//

//
// Created by tumbar on 12/23/20.
//

#include <lexer.h>
#include <parser.h>
#include <cmocka.h>
#include <stdlib.h>

#define CTEST(name) static void name(void** state)

#define LR_S(i) ((i) | TOK_SHIFT_MASK)
#define LR_R(i) ((i) | TOK_REDUCE_MASK)
#define LR_E() TOK_SYNTAX_ERROR
#define LR_A() TOK_ACCEPT_MASK

/**
 * S -> AA
 * A -> aA
 * A -> b
 */

enum
{
    // 0 reserved for eof
    TOK_a = 1,
    TOK_b,
    TOK_S,
    TOK_A
};

typedef union {
    int integer;
    struct expr_ {
        int val;
        struct expr_* next;
    }* expr;
} ValUnion;

I32 ll_skip(const char* yytext, void* yyval)
{
    (void)yytext;
    (void)yyval;
    return -1; // skip
}
I32 ll_tok_num(const char* yytext, ValUnion* yyval)
{
    yyval->integer = (int)strtol(yytext, NULL, 10);
    return TOK_a;
}
I32 ll_tok_term(const char* yytext, ValUnion* yyval)
{
    (void)yytext;
    (void)yyval;
    return TOK_b;
}

U32 gg_lr_r1(ValUnion* dest, ValUnion* args)
{
    (void)dest;
    (void)args;
    return TOK_S;
}

U32 gg_lr_r2(ValUnion* dest, ValUnion* args)
{
    (void)dest;
    (void)args;
    return TOK_A;
}

U32 gg_lr_r3(ValUnion* dest, ValUnion* args)
{
    (void)dest;
    (void)args;
    return TOK_A;
}

static GrammarParser p;

static const
U32 lalr_table[] = {
        LR_E( ), LR_S(3), LR_S(4), LR_S(1), LR_S(2), /* 0 */
        LR_A( ), LR_A( ), LR_A( ), LR_E( ), LR_E( ), /* 1 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(5), /* 2 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(6), /* 3 */
        LR_R(3), LR_R(3), LR_R(3), LR_E( ), LR_E( ), /* 4 */
        LR_R(1), LR_E( ), LR_E( ), LR_E( ), LR_E( ), /* 5 */
        LR_R(2), LR_R(2), LR_R(2), LR_E( ), LR_E( ), /* 6 */
};

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.expr = (lexer_expr) ll_tok_term, .regex = 0},
            {.expr = ll_skip, .regex = 0},
            {.expr = (lexer_expr) ll_tok_num, .regex = 0}
    };

    static U32 r1[] = {
            TOK_A,
            TOK_A
    };

    static U32 r2[] = {
            TOK_a,
            TOK_A
    };

    static U32 r3[] = {
            TOK_b
    };

    static GrammarRule g_rules[] = {
            {}, // Place holder for reduction offsets
            {.tok_n = 2, .grammar = r1, .expr = (parser_expr) gg_lr_r1},
            {.tok_n = 2, .grammar = r2, .expr = (parser_expr) gg_lr_r2},
            {.tok_n = 1, .grammar = r3, .expr = (parser_expr) gg_lr_r3},
    };

    p.grammar_n = 4;
    p.grammar_rules = g_rules;
    p.lex_n = 3;
    p.lexer_rules = l_rules;
    p.col_count = 5,
    p.parser_table = lalr_table;

    // Initialize the lexer regex rules
    assert_int_equal(regcomp(&l_rules[0].regex, "^;", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[1].regex, "^[ ]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[2].regex, "^[0-9]+", REG_EXTENDED), 0);
}

CTEST(test_parser)
{
    const char* lexer_input = ";"   // b
                              "10 " // a
                              "20 " // a
                              "30"  // a
                              ";";  // b
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    ValUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 5);

    ParserStack* stack = malloc(sizeof(ParserStack) + (sizeof(U32) * 64));
    stack->pos = 0;
    I32 res_idx = parser_parse_lr(&p, stack, token_table, value_table, sizeof(ValUnion));

    free(stack);

    assert_int_not_equal(res_idx, -1);
}


const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_parser),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}