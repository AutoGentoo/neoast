//
// Created by tumbar on 12/23/20.
//

#include <lexer.h>
#include <parser.h>
#include <cmocka.h>
#include <stdlib.h>
#include <parsergen/canonical_collection.h>
#include <parsergen/clr_lalr.h>
#include <math.h>
#include <util/util.h>

#define CTEST(name) static void name(void** state)

#define LR_S(i) ((i) | TOK_SHIFT_MASK)
#define LR_R(i) ((i) | TOK_REDUCE_MASK)
#define LR_E() TOK_SYNTAX_ERROR
#define LR_A() TOK_ACCEPT_MASK

/**
 *
 * S -> E
 *      | [empty]
 * E -> NUM
 *      | E '+' E
 *      | E '-' E
 *      | E '*' E
 *      | E '/' E
 *      | E '^' E
 *      | '(' E ')'
 */

enum
{
    TOK_EOF = 0,
    TOK_NUM,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_CARET,
    TOK_OPEN_P,
    TOK_CLOSE_P,
    TOK_E,
    TOK_S,
    TOK_AUGMENT
};

static const char* token_error_names[] = {
        "EOF",
        "number",
        "+",
        "-",
        "*",
        "/",
        "^",
        "(",
        ")",
        "expression",
        "input",
        "augment"
};

static const char* token_names = "$N+-*/^()ES'";
static U8 precedence_table[TOK_AUGMENT] = {PRECEDENCE_NONE};

typedef union {
    double number;
    char operator;
} LexerUnion;

I32 ll_tok_operator(const char* yytext, LexerUnion* yyval)
{
    yyval->operator = *yytext;
    switch (*yytext)
    {
        case '+': return TOK_PLUS;
        case '-': return TOK_MINUS;
        case '*': return TOK_STAR;
        case '/': return TOK_SLASH;
        case '^': return TOK_CARET;
        case '(': return TOK_OPEN_P;
        case ')': return TOK_CLOSE_P;
    }

    return -1;
}
I32 ll_tok_num(const char* yytext, LexerUnion* yyval)
{
    yyval->number = strtod(yytext, NULL);
    return TOK_NUM;
}

void binary_op(LexerUnion* dest, LexerUnion* args)
{
    switch (args[1].operator)
    {
        case '+': dest->number = args[0].number + args[2].number; return;
        case '-': dest->number = args[0].number - args[2].number; return;
        case '*': dest->number = args[0].number * args[2].number; return;
        case '/': dest->number = args[0].number / args[2].number; return;
        case '^': dest->number = pow(args[0].number, args[2].number); return;
    }
}

void group_op(LexerUnion* dest, LexerUnion* args)
{
    dest->number = args[1].number;
}

void copy_op(LexerUnion* dest, LexerUnion* args)
{
    dest->number = args[0].number;
}

static GrammarParser p;

void initialize_parser()
{
    static LexerRule l_rules_s0[] = {
            {.regex_raw = "^[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex_raw = "^[0-9]+"},
            {.expr = (lexer_expr) ll_tok_operator, .regex_raw = "^[\\(\\)\\+\\-\\*\\/\\^]"}
    };

    /*
     * E -> NUM
     *      | E '+' E
     *      | E '-' E
     *      | E '*' E
     *      | E '/' E
     *      | E '^' E
     *      | '(' E ')'
     */

    static U32 r1[] = {TOK_NUM};
    static U32 r2[] = {TOK_E, TOK_PLUS, TOK_E};
    static U32 r3[] = {TOK_E, TOK_MINUS, TOK_E};
    static U32 r4[] = {TOK_E, TOK_STAR, TOK_E};
    static U32 r5[] = {TOK_E, TOK_SLASH, TOK_E};
    static U32 r6[] = {TOK_E, TOK_CARET, TOK_E};
    static U32 r7[] = {TOK_OPEN_P, TOK_E, TOK_CLOSE_P};
    static U32 r8[] = {TOK_E};
    static U32 r9[] = {};

    precedence_table[TOK_PLUS] = PRECEDENCE_LEFT;
    precedence_table[TOK_MINUS] = PRECEDENCE_LEFT;
    precedence_table[TOK_STAR] = PRECEDENCE_LEFT;
    precedence_table[TOK_SLASH] = PRECEDENCE_LEFT;
    precedence_table[TOK_CARET] = PRECEDENCE_RIGHT;

    static U32 a_r[] = {TOK_S};

    static GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r, .expr = NULL}, // Augmented rule
            {.token = TOK_S, .tok_n = 1, .grammar = r8, .expr = (parser_expr) copy_op},
            {.token = TOK_S, .tok_n = 0, .grammar = r9, .expr = NULL}, // Empty, no-op
            {.token = TOK_E, .tok_n = 3, .grammar = r7, .expr = (parser_expr) group_op},
            {.token = TOK_E, .tok_n = 1, .grammar = r1, .expr = (parser_expr) copy_op},
            {.token = TOK_E, .tok_n = 3, .grammar = r6, .expr = (parser_expr) binary_op},
            {.token = TOK_E, .tok_n = 3, .grammar = r5, .expr = (parser_expr) binary_op},
            {.token = TOK_E, .tok_n = 3, .grammar = r4, .expr = (parser_expr) binary_op},
            {.token = TOK_E, .tok_n = 3, .grammar = r3, .expr = (parser_expr) binary_op},
            {.token = TOK_E, .tok_n = 3, .grammar = r2, .expr = (parser_expr) binary_op},
    };

    static LexerRule* l_rules[] = {
            l_rules_s0
    };

    static U32 lex_n = ARR_LEN(l_rules_s0);

    p.grammar_n = 10;
    p.grammar_rules = g_rules;
    p.lex_state_n = 1;
    p.lex_n = &lex_n;
    p.lexer_rules = l_rules;
    p.action_token_n = 9;
    p.token_n = TOK_AUGMENT;

    // Initialize the lexer regex rules
    parser_init(&p);
}

CTEST(test_clr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, CLR_1);

    U32* table = canonical_collection_generate(cc, precedence_table);
    dump_table(table, cc, token_names, 0);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    U32* table = canonical_collection_generate(cc, precedence_table);
    dump_table(table, cc, token_names, 0);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1_calculator)
{
    const char* lexer_input = "1 + (5 * 9) + 2";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    LexerUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(LexerUnion), 32);
    assert_int_equal(tok_n, 9);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    U32* table = canonical_collection_generate(cc, precedence_table);

    Stack* stack = malloc(sizeof(Stack) + (sizeof(U32) * 64));
    stack->pos = 0;
    I32 res_idx = parser_parse_lr(&p, stack, table, token_error_names, token_table, value_table, sizeof(LexerUnion));

    dump_table(table, cc, token_names, 0);
    assert_int_not_equal(res_idx, -1);

    printf("%s = %lf\n", lexer_input, value_table[res_idx].number);
    assert_double_equal(value_table[res_idx].number, 1 + (5 * 9) + 2, 0.005);

    free(stack);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1_order_of_ops)
{
    const char* lexer_input = "1 + 5 * 9 + 4";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    LexerUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(LexerUnion), 32);
    assert_int_equal(tok_n, 7);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    U32* table = canonical_collection_generate(cc, precedence_table);

    Stack* stack = malloc(sizeof(Stack) + (sizeof(U32) * 64));
    stack->pos = 0;
    I32 res_idx = parser_parse_lr(&p, stack, table, token_error_names, token_table, value_table, sizeof(LexerUnion));

    // This parser has no order of ops
    assert_double_equal(value_table[res_idx].number, (((1 + 5) * 9) + 4), 0.005);

    free(stack);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}


CTEST(test_first_of_expr)
{
    initialize_parser();

    U8 first_of_items[TOK_E] = {0};
    lr_1_firstof(first_of_items, TOK_E, &p);

    assert_true(first_of_items[TOK_OPEN_P]);
    assert_true(first_of_items[TOK_NUM]);

    assert_false(first_of_items[TOK_EOF]);
    assert_false(first_of_items[TOK_CLOSE_P]);
    assert_false(first_of_items[TOK_PLUS]);
    assert_false(first_of_items[TOK_MINUS]);
    assert_false(first_of_items[TOK_STAR]);
    assert_false(first_of_items[TOK_SLASH]);
    assert_false(first_of_items[TOK_CARET]);
}

const static struct CMUnitTest left_scan_tests[] = {
        //cmocka_unit_test(test_first_of_expr),
        //cmocka_unit_test(test_clr_1),
        //cmocka_unit_test(test_lalr_1),
        cmocka_unit_test(test_lalr_1_calculator),
        //cmocka_unit_test(test_lalr_1_order_of_ops),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}