//
// Created by tumbar on 12/23/20.
//

#include <lexer.h>
#include <parser.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

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

typedef union {
    int integer;
    struct expr_ {
        int val;
        struct expr_* next;
    }* expr;
} CodegenUnion;

I32 ll_tok_num(const char* yytext, CodegenUnion* yyval)
{
    yyval->integer = (int)strtol(yytext, NULL, 10);
    return TOK_a;
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
            {.tok = TOK_b, .regex_raw = ";"},
            {.regex_raw = "[ ]+"},
            {.expr = (lexer_expr) ll_tok_num, .regex_raw = "[0-9]+"}
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

    static U32 a_r[] = {
            TOK_AUGMENT
    };

    static GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r},
            {.token = TOK_S, .tok_n = 2, .grammar = r1},
            {.token = TOK_A, .tok_n = 2, .grammar = r2},
            {.token = TOK_A, .tok_n = 1, .grammar = r3},
    };

    static LexerRule* rules[] = {l_rules};

    static U32 lex_n = ARR_LEN(l_rules);

    p.grammar_n = 4;
    p.grammar_rules = g_rules;
    p.lex_n = &lex_n;
    p.lex_state_n = 1;
    p.lexer_rules = rules;
    p.token_n = TOK_AUGMENT;
    p.action_token_n = 3;
    p.token_names = token_error_names;

    // Initialize the lexer regex rules
    parser_init(&p);
}

CTEST(test_parser)
{
    const char* lexer_input = ";"   // b
                              "10 " // a
                              "20 " // a
                              "30"  // a
                              ";";  // b
    initialize_parser();

    ParserBuffers* buf = parser_allocate_buffers(32, 32, 4, sizeof(CodegenUnion));

    int tok_n = lexer_fill_table(lexer_input, strlen(lexer_input), &p, buf);
    assert_int_equal(tok_n, 5);

    I32 res_idx = parser_parse_lr(&p, lalr_table, buf);

    parser_free_buffers(buf);
    parser_free(&p);
    assert_int_not_equal(res_idx, -1);
}


const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_parser),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}