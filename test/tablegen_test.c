//
// Created by tumbar on 12/23/20.
//

#include <lexer.h>
#include <parser.h>
#include <cmocka.h>
#include <stdlib.h>
#include <parser_gen/canonical_collection.h>
#include <parser_gen/clr_lalr.h>

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
    // 0 reserved for eof
    TOK_NUM = 1,
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

typedef union {
    double number;
} ValUnion;

I32 ll_skip(const char* yytext, void* yyval)
{
    (void)yytext;
    (void)yyval;
    return -1; // skip
}

I32 ll_tok_operator(const char* yytext, void* yyval)
{
    (void)yyval;

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
I32 ll_tok_num(const char* yytext, ValUnion* yyval)
{
    yyval->number = strtod(yytext, NULL);
    return TOK_NUM;
}

static GrammarParser p;

void dump_table(U32* table, int row_n, int col_n, int divider)
{
    U32 i = 0;
    printf("token    : ");
    for (U32 col = 0; col < col_n; col++)
    {
        if (col == divider)
            printf("|");
        printf("  %02d |", col);
    }
    printf("\n");

    for (U32 row = 0; row < row_n; row++)
    {
        printf("state %02d : ", row);
        for (U32 col = 0; col < col_n; col++, i++)
        {
            if (col == divider)
                printf("|");

            if (!table[i])
                printf("  E  |");
            else if (table[i] & TOK_ACCEPT_MASK)
                printf("  A  |");
            else if (table[i] & TOK_SHIFT_MASK)
            {
                printf(" S%02d |", table[i] & TOK_MASK);
            }
            else if (table[i] & TOK_REDUCE_MASK)
            {
                printf(" R%02d |", table[i] & TOK_MASK);
            }
        }
        printf("\n");
    }
}

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.expr = ll_skip, .regex = 0},
            {.expr = (lexer_expr) ll_tok_num, .regex = 0},
            {.expr = ll_tok_operator, .regex = 0}
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

    static U32 r2[] = {TOK_NUM, TOK_PLUS, TOK_E};
    static U32 r3[] = {TOK_NUM, TOK_MINUS, TOK_E};
    static U32 r4[] = {TOK_NUM, TOK_STAR, TOK_E};
    static U32 r5[] = {TOK_NUM, TOK_SLASH, TOK_E};
    static U32 r6[] = {TOK_NUM, TOK_CARET, TOK_E};
    static U32 r7[] = {TOK_OPEN_P, TOK_E, TOK_CLOSE_P};
    static U32 r8[] = {TOK_E};
    static U32 r9[] = {};

    static U32 a_r[] = {TOK_S};

    static GrammarRule g_rules[] = {
            {.token = TOK_S, .tok_n = 0, .grammar = r9, .expr = NULL},
            {.token = TOK_S, .tok_n = 1, .grammar = r8, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r7, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r6, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r5, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r4, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r3, .expr = NULL},
            {.token = TOK_E, .tok_n = 3, .grammar = r2, .expr = NULL},
    };

    static GrammarRule aug_rule = {
            .tok_n = 1,
            .token = TOK_AUGMENT,
            .expr = NULL,
            .grammar = a_r
    };

    p.grammar_n = 8;
    p.grammar_rules = g_rules;
    p.lex_n = 3;
    p.lexer_rules = l_rules;
    p.augmented_rule = &aug_rule;
    p.action_token_n = 8;
    p.token_n = TOK_AUGMENT;

    // Initialize the lexer regex rules
    assert_int_equal(regcomp(&l_rules[0].regex, "^[ ]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[1].regex, "^[0-9]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[2].regex, "^[\\(\\)\\+\\-\\*\\/\\^]", REG_EXTENDED), 0);
}

CTEST(test_clr_1)
{
    const char* lexer_input = "1 + 5 * 9 + 2";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    ValUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 7);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, clr_1_cmp, clr_1_merge);

    U32* table = canonical_collection_generate(cc);
    dump_table(table, cc->state_n, cc->parser->token_n, cc->parser->action_token_n);
    canonical_collection_free(cc);
    free(table);
}

CTEST(test_lalr_1)
{
    const char* lexer_input = "1 + 5 * 9 + 2";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    ValUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 7);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, lalr_1_cmp, lalr_1_merge);

    U32* table = canonical_collection_generate(cc);
    dump_table(table, cc->state_n, cc->parser->token_n, cc->parser->action_token_n);
    canonical_collection_free(cc);
    free(table);
}


const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_clr_1),
        cmocka_unit_test(test_lalr_1),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}