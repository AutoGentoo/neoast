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


#include <lexer.h>
#include <cmocka.h>
#include <stdlib.h>
#include <parsergen/canonical_collection.h>
#include <math.h>
#include <util/util.h>
#include <string.h>

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
static uint8_t precedence_table[TOK_AUGMENT] = {PRECEDENCE_NONE};

typedef union {
    double number;
    char operator;
} CalculatorUnion;

int32_t ll_tok_operator(const char* yytext, CalculatorUnion* yyval)
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
int32_t ll_tok_num(const char* yytext, CalculatorUnion* yyval)
{
    yyval->number = strtod(yytext, NULL);
    return TOK_NUM;
}

void binary_op(CalculatorUnion* dest, CalculatorUnion* args)
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

void group_op(CalculatorUnion* dest, CalculatorUnion* args)
{
    dest->number = args[1].number;
}

void copy_op(CalculatorUnion* dest, CalculatorUnion* args)
{
    dest->number = args[0].number;
}

static GrammarParser p;

void initialize_parser()
{
    static LexerRule l_rules_s0[] = {
            {.regex_raw = "[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex_raw = "[0-9]+"},
            {.expr = (lexer_expr) ll_tok_operator, .regex_raw = "[\\(\\)\\+\\-\\*\\/\\^]"}
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

    static uint32_t r1[] = {TOK_NUM};
    static uint32_t r2[] = {TOK_E, TOK_PLUS, TOK_E};
    static uint32_t r3[] = {TOK_E, TOK_MINUS, TOK_E};
    static uint32_t r4[] = {TOK_E, TOK_STAR, TOK_E};
    static uint32_t r5[] = {TOK_E, TOK_SLASH, TOK_E};
    static uint32_t r6[] = {TOK_E, TOK_CARET, TOK_E};
    static uint32_t r7[] = {TOK_OPEN_P, TOK_E, TOK_CLOSE_P};
    static uint32_t r8[] = {TOK_E};
    static uint32_t r9[] = {};

    precedence_table[TOK_PLUS] = PRECEDENCE_LEFT;
    precedence_table[TOK_MINUS] = PRECEDENCE_LEFT;
    precedence_table[TOK_STAR] = PRECEDENCE_LEFT;
    precedence_table[TOK_SLASH] = PRECEDENCE_LEFT;
    precedence_table[TOK_CARET] = PRECEDENCE_RIGHT;

    static uint32_t a_r[] = {TOK_S};

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

    static uint32_t lex_n = NEOAST_ARR_LEN(l_rules_s0);

    p.grammar_n = 10;
    p.grammar_rules = g_rules;
    p.lex_state_n = 1;
    p.lex_n = &lex_n;
    p.lexer_rules = l_rules;
    p.action_token_n = 9;
    p.token_n = TOK_AUGMENT;
    p.token_names = token_error_names;

    // Initialize the lexer regex rules
    parser_init(&p);
}

CTEST(test_clr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, CLR_1);

    uint8_t error;
    uint32_t* table = canonical_collection_generate(cc, precedence_table, &error);
    assert_int_equal(error, 0);

    dump_table(table, cc, token_names, 0, stdout, NULL);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    uint8_t error;
    uint32_t* table = canonical_collection_generate(cc, precedence_table, &error);
    assert_int_equal(error, 0);

    dump_table(table, cc, token_names, 0, stdout, NULL);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1_calculator)
{
    const char* lexer_input = "1 + (5 * 9) + 2";
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(32, 32, 4, 1024, sizeof(CalculatorUnion), sizeof(CalculatorUnion));

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    uint8_t error;
    uint32_t* table = canonical_collection_generate(cc, precedence_table, &error);
    assert_int_equal(error, 0);

    int32_t res_idx = parser_parse_lr(&p, table, buf, lexer_input, strlen(lexer_input));

    dump_table(table, cc, token_names, 0, stdout, NULL);
    assert_int_not_equal(res_idx, -1);

    printf("%s = %lf\n", lexer_input, ((CalculatorUnion*)buf->value_table)[res_idx].number);
    assert_double_equal(((CalculatorUnion*)buf->value_table)[res_idx].number, 1 + (5 * 9) + 2, 0.005);

    parser_free_buffers(buf);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

CTEST(test_lalr_1_order_of_ops)
{
    const char* lexer_input = "1 + 5 * 9 + 4";
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(32, 32, 4, 1024, sizeof(CalculatorUnion), sizeof(CalculatorUnion));

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, LALR_1);

    uint8_t error;
    uint32_t* table = canonical_collection_generate(cc, precedence_table, &error);
    assert_int_equal(error, 0);

    int32_t res_idx = parser_parse_lr(&p, table, buf, lexer_input, strlen(lexer_input));

    // This parser has no order of ops
    assert_double_equal(((CalculatorUnion*)buf->value_table)[res_idx].number, (((1 + 5) * 9) + 4), 0.005);

    parser_free_buffers(buf);
    canonical_collection_free(cc);
    free(table);
    parser_free(&p);
}

uint8_t lr_1_firstof(uint8_t dest[],
                     uint32_t token,
                     const GrammarParser* parser);

CTEST(test_first_of_expr)
{
    initialize_parser();

    uint8_t first_of_items[TOK_E] = {0};
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
    parser_free(&p);
}

const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_first_of_expr),
        cmocka_unit_test(test_clr_1),
        cmocka_unit_test(test_lalr_1),
        cmocka_unit_test(test_lalr_1_calculator),
        cmocka_unit_test(test_lalr_1_order_of_ops),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}
