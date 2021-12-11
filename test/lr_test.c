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
#include <string.h>
#include <neoast.h>
#include <codegen/bootstrap/lexer/bootstrap_lexer.h>
#include <cmocka.h>
#include <stddef.h>

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
    TOK_A,
    TOK_AUGMENT
};

static const char* token_error_names[] = {
        "EOF",
        "a",
        "b",
        "S",
        "A",
        "augment"
};

typedef union
{
    int integer;
    struct expr_
    {
        int val;
        struct expr_* next;
    } * expr;
} CodegenUnion;

typedef struct
{
    CodegenUnion value;
    TokenPosition position;
} CodegenStruct;

int32_t ll_tok_num(const char* yytext, CodegenUnion* yyval)
{
    yyval->integer = (int) strtol(yytext, NULL, 10);
    return TOK_a;
}

static GrammarParser p;
static void* lexer_parent;

static const
uint32_t lalr_table[] = {
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
            {.tok = TOK_b, .regex = ";"},
            {.regex = "[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex = "[0-9]+"}
    };

    static uint32_t r1[] = {
            TOK_A,
            TOK_A
    };

    static uint32_t r2[] = {
            TOK_a,
            TOK_A
    };

    static uint32_t r3[] = {
            TOK_b
    };

    static uint32_t a_r[] = {
            TOK_AUGMENT
    };

    static GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r},
            {.token = TOK_S, .tok_n = 2, .grammar = r1},
            {.token = TOK_A, .tok_n = 2, .grammar = r2},
            {.token = TOK_A, .tok_n = 1, .grammar = r3},
    };

    static LexerRule* rules[] = {l_rules};

    static uint32_t lex_n = NEOAST_ARR_LEN(l_rules);

    p.grammar_n = 4;
    p.grammar_rules = g_rules;
    p.token_n = TOK_AUGMENT;
    p.action_token_n = 3;
    p.token_names = token_error_names;

    // Initialize the lexer regex rules
    lexer_parent = bootstrap_lexer_new((const LexerRule**) rules, &lex_n, 1, NULL,
                                       offsetof(CodegenStruct, position), NULL);
}

CTEST(test_parser)
{
    const char* lexer_input = ";"   // b
                              "10 " // a
                              "20 " // a
                              "30"  // a
                              ";";  // b
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(256, 256, sizeof(CodegenUnion), sizeof(CodegenUnion));

    void* lexer_inst = bootstrap_lexer_instance_new(lexer_parent, lexer_input, strlen(lexer_input));
    int32_t res_idx = parser_parse_lr(&p, lalr_table, buf, lexer_inst, bootstrap_lexer_next);
    bootstrap_lexer_instance_free(lexer_inst);

    parser_free_buffers(buf);
    bootstrap_lexer_free(lexer_parent);
    assert_int_not_equal(res_idx, -1);
}


const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_parser),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}
