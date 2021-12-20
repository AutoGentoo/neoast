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


#include <stdlib.h>
#include <parsergen/c_pub.h>
#include <math.h>
#include <util/util.h>
#include <string.h>
#include "lexer/bootstrap_lexer.h"
#include <cmocka.h>
#include <stddef.h>

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
    TOK_EOF = NEOAST_ASCII_MAX,
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
        "$",
        "n",
        "+",
        "-",
        "*",
        "/",
        "^",
        "(",
        ")",
        "E",
        "I",
        "A"
};

static const char* token_names[] = {
        "$",
        "N",
        "+",
        "-",
        "*",
        "/",
        "^",
        "(",
        ")",
        "E",
        "S",
        "'"
};
static uint8_t precedence_table[TOK_AUGMENT - NEOAST_ASCII_MAX] = {PRECEDENCE_NONE};

typedef union
{
    double number;
    char operator;
} CalculatorUnion;

typedef struct
{
    CalculatorUnion value;
    TokenPosition position;
} CalculatorStruct;

int32_t ll_tok_operator(const char* yytext, CalculatorUnion* yyval)
{
    yyval->operator = *yytext;
    switch (*yytext)
    {
        case '+':
            return TOK_PLUS;
        case '-':
            return TOK_MINUS;
        case '*':
            return TOK_STAR;
        case '/':
            return TOK_SLASH;
        case '^':
            return TOK_CARET;
        case '(':
            return TOK_OPEN_P;
        case ')':
            return TOK_CLOSE_P;
    }

    return -1;
}

int32_t ll_tok_num(const char* yytext, CalculatorUnion* yyval)
{
    yyval->number = strtod(yytext, NULL);
    return TOK_NUM;
}

void binary_op(CalculatorStruct* dest, CalculatorStruct* args)
{
    switch (args[1].value.operator)
    {
        case '+':
            dest->value.number = args[0].value.number + args[2].value.number;
            return;
        case '-':
            dest->value.number = args[0].value.number - args[2].value.number;
            return;
        case '*':
            dest->value.number = args[0].value.number * args[2].value.number;
            return;
        case '/':
            dest->value.number = args[0].value.number / args[2].value.number;
            return;
        case '^':
            dest->value.number = pow(args[0].value.number, args[2].value.number);
            return;
    }
}

void group_op(CalculatorStruct* dest, CalculatorStruct* args)
{
    dest->value.number = args[1].value.number;
}

void copy_op(CalculatorStruct* dest, CalculatorStruct* args)
{
    dest->value.number = args[0].value.number;
}

static GrammarParser p;
static void* lexer_parent;

static void (* const reduce_table[])(CalculatorStruct*, CalculatorStruct*) = {
        NULL, copy_op,
        NULL, group_op,
        copy_op, binary_op,
        binary_op, binary_op,
        binary_op, binary_op
};

void reduce_handler(uint32_t reduce_id, CalculatorStruct* dest, CalculatorStruct* args)
{
    if (reduce_table[reduce_id])
    {
        reduce_table[reduce_id](dest, args);
    }
    else
    {
        *dest = args[0];
    }
}

void initialize_parser()
{
    static LexerRule l_rules_s0[] = {
            {.regex = "[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex = "[0-9]+"},
            {.expr = (lexer_expr) ll_tok_operator, .regex = "[\\(\\)\\+\\-\\*\\/\\^]"}
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

    precedence_table[TOK_PLUS - NEOAST_ASCII_MAX] = PRECEDENCE_LEFT;
    precedence_table[TOK_MINUS - NEOAST_ASCII_MAX] = PRECEDENCE_LEFT;
    precedence_table[TOK_STAR - NEOAST_ASCII_MAX] = PRECEDENCE_LEFT;
    precedence_table[TOK_SLASH - NEOAST_ASCII_MAX] = PRECEDENCE_LEFT;
    precedence_table[TOK_CARET - NEOAST_ASCII_MAX] = PRECEDENCE_RIGHT;

    static uint32_t a_r[] = {TOK_S};

    static GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r}, // Augmented rule
            {.token = TOK_S, .tok_n = 1, .grammar = r8},
            {.token = TOK_S, .tok_n = 0, .grammar = r9}, // Empty, no-op
            {.token = TOK_E, .tok_n = 3, .grammar = r7},
            {.token = TOK_E, .tok_n = 1, .grammar = r1},
            {.token = TOK_E, .tok_n = 3, .grammar = r6},
            {.token = TOK_E, .tok_n = 3, .grammar = r5},
            {.token = TOK_E, .tok_n = 3, .grammar = r4},
            {.token = TOK_E, .tok_n = 3, .grammar = r3},
            {.token = TOK_E, .tok_n = 3, .grammar = r2},
    };

    static LexerRule* l_rules[] = {
            l_rules_s0
    };

    static uint32_t lex_n = NEOAST_ARR_LEN(l_rules_s0);

    p.grammar_n = 10;
    p.grammar_rules = g_rules;
    p.action_token_n = 9;
    p.token_n = TOK_AUGMENT - NEOAST_ASCII_MAX;
    p.token_names = token_error_names;
    p.parser_reduce = (parser_reduce) reduce_handler;

    lexer_parent = bootstrap_lexer_new((const LexerRule**) l_rules, &lex_n, 1, NULL,
                                       offsetof(CalculatorStruct, position), NULL);
}

CTEST(test_clr_1)
{
    initialize_parser();
    void* cc = canonical_collection_init(&p, NULL);
    canonical_collection_resolve(cc, CLR_1);

    uint32_t* table = malloc(sizeof(uint32_t) * canonical_collection_table_size(cc));
    uint32_t error = canonical_collection_generate(cc, table, precedence_table);
    assert_int_equal(error, 0);

    dump_table(table, cc, token_names, 0, stdout, NULL);
    canonical_collection_free(cc);
    free(table);
    bootstrap_lexer_free(lexer_parent);
}

CTEST(test_lalr_1)
{
    initialize_parser();
    void* cc = canonical_collection_init(&p, NULL);
    canonical_collection_resolve(cc, LALR_1);

    uint32_t* table = malloc(sizeof(uint32_t) * canonical_collection_table_size(cc));
    uint32_t error = canonical_collection_generate(cc, table, precedence_table);
    assert_int_equal(error, 0);

    dump_table(table, cc, token_names, 0, stdout, NULL);
    canonical_collection_free(cc);
    free(table);
    bootstrap_lexer_free(lexer_parent);
}

CTEST(test_lalr_1_calculator)
{
    const char* lexer_input = "1 + (5 * 9) + 2";
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(256, 256, sizeof(CalculatorStruct),
                                                 offsetof(CalculatorStruct, position));

    void* cc = canonical_collection_init(&p, NULL);
    canonical_collection_resolve(cc, LALR_1);

    uint32_t* table = malloc(sizeof(uint32_t) * canonical_collection_table_size(cc));
    uint32_t error = canonical_collection_generate(cc, table, precedence_table);
    assert_int_equal(error, 0);

    void* lexer_inst = bootstrap_lexer_instance_new(lexer_parent, lexer_input, strlen(lexer_input));
    int32_t res_idx = parser_parse_lr(&p, NULL, table, buf, lexer_inst, bootstrap_lexer_next);
    bootstrap_lexer_instance_free(lexer_inst);

    dump_table(table, cc, token_names, 0, stdout, NULL);
    assert_int_not_equal(res_idx, -1);

    printf("%s = %lf\n", lexer_input, ((CalculatorStruct*) buf->value_table)[res_idx].value.number);
    assert_double_equal(((CalculatorStruct*) buf->value_table)[res_idx].value.number, 1 + (5 * 9) + 2, 0.005);

    parser_free_buffers(buf);
    canonical_collection_free(cc);
    free(table);
    bootstrap_lexer_free(lexer_parent);
}

CTEST(test_lalr_1_order_of_ops)
{
    const char* lexer_input = "1 + 5 * 9 + 4";
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(256, 256, sizeof(CalculatorStruct),
                                                 offsetof(CalculatorStruct, position));

    void* cc = canonical_collection_init(&p, NULL);
    canonical_collection_resolve(cc, LALR_1);

    uint32_t* table = malloc(sizeof(uint32_t) * canonical_collection_table_size(cc));
    uint32_t error = canonical_collection_generate(cc, table, precedence_table);
    assert_int_equal(error, 0);

    void* lexer_inst = bootstrap_lexer_instance_new(lexer_parent, lexer_input, strlen(lexer_input));
    int32_t res_idx = parser_parse_lr(&p, NULL, table, buf, lexer_inst, bootstrap_lexer_next);
    bootstrap_lexer_instance_free(lexer_inst);

    // This parser has no order of ops
    assert_int_not_equal(res_idx, -1);
    assert_double_equal(((CalculatorStruct*) buf->value_table)[res_idx].value.number, (((1 + 5) * 9) + 4), 0.005);

    parser_free_buffers(buf);
    canonical_collection_free(cc);
    free(table);
    bootstrap_lexer_free(lexer_parent);
}

const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_clr_1),
        cmocka_unit_test(test_lalr_1),
        cmocka_unit_test(test_lalr_1_calculator),
        cmocka_unit_test(test_lalr_1_order_of_ops),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}
