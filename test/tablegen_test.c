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

static const char* token_names = "$N+-*/^()ES'";
static precedence_t precedence_table[TOK_AUGMENT] = {PRECEDENCE_NONE};

typedef union {
    double number;
    char operator;
} ValUnion;

I32 ll_skip(const char* yytext, void* yyval)
{
    (void)yytext;
    (void)yyval;
    return -1; // skip
}

I32 ll_tok_operator(const char* yytext, ValUnion* yyval)
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
I32 ll_tok_num(const char* yytext, ValUnion* yyval)
{
    yyval->number = strtod(yytext, NULL);
    return TOK_NUM;
}

void binary_op(ValUnion* dest, ValUnion* args)
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

void group_op(ValUnion* dest, ValUnion* args)
{
    dest->number = args[1].number;
}

void copy_op(ValUnion* dest, ValUnion* args)
{
    dest->number = args[0].number;
}

static GrammarParser p;

void dump_item(const LR_1* lr1, U32 tok_n, const char* tok_names)
{
    U8 printed = 0;
    printf("['");
    for (U32 i = 0; i < lr1->grammar->tok_n; i++)
    {
        if (i == lr1->item_i && !printed)
        {
            printed = 1;
            printf("•");
        }

        printf("%c", tok_names[lr1->grammar->grammar[i]]);
    }

    if (lr1->final_item)
        printf("•");

    printf("'");

    if (lr1->final_item)
        printf("!");

    printf(",");

    // Print the lookaheads
    printed = 0;
    for (U32 i = 0; i < tok_n; i++)
    {
        if (lr1->look_ahead[i])
        {
            if (printed)
            {
                printf("/");
            }

            printf("%c", tok_names[i]);
            printed = 1;
        }
    }
    printf("]; ");
}

void dump_state(const GrammarState* state, U32 tok_n, const char* tok_names, U8 line_wrap)
{
    U8 is_first = 1;
    for (const LR_1* item = state->head_item; item; item = item->next)
    {
        if (!is_first && line_wrap)
        {
            // Print spacing
            printf("%*c", 12 + (6 * tok_n), ' ');
        }

        is_first = 0;
        dump_item(item, tok_n, tok_names);

        if (line_wrap)
        {
            printf("\n");
        }
    }
}

void dump_table(const U32* table,
                const CanonicalCollection* cc,
                const char* tok_names,
                U8 state_wrap)
{
    U32 i = 0;
    printf("token_id : ");
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            printf("|");
        printf(" %*d  |", 2, col);
    }
    printf("\n");

    printf("token    : ");
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            printf("|");
        printf("  %c  |", tok_names[col]);
    }
    printf("\n");
    printf("___________");
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            printf("_");
        printf("______");
    }
    printf("\n");

    for (U32 row = 0; row < cc->state_n; row++)
    {
        printf("state %02d : ", row);
        for (U32 col = 0; col < cc->parser->token_n; col++, i++)
        {
            if (col == cc->parser->action_token_n)
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

        dump_state(cc->all_states[row], cc->parser->token_n, tok_names, state_wrap);

        if (!state_wrap)
            printf("\n");
    }
}

void initialize_parser()
{
    static LexerRule l_rules[] = {
            {.expr = ll_skip, .regex = 0},
            {.expr = (lexer_expr) ll_tok_num, .regex = 0},
            {.expr = (lexer_expr) ll_tok_operator, .regex = 0}
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

    p.grammar_n = 10;
    p.grammar_rules = g_rules;
    p.lex_n = 3;
    p.lexer_rules = l_rules;
    p.action_token_n = 9;
    p.token_n = TOK_AUGMENT;

    // Initialize the lexer regex rules
    assert_int_equal(regcomp(&l_rules[0].regex, "^[ ]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[1].regex, "^[0-9]+", REG_EXTENDED), 0);
    assert_int_equal(regcomp(&l_rules[2].regex, "^[\\(\\)\\+\\-\\*\\/\\^]", REG_EXTENDED), 0);
}

CTEST(test_clr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, clr_1_cmp, clr_1_merge);

    U32* table = canonical_collection_generate(cc, precedence_table);
    dump_table(table, cc, token_names, 0);
    canonical_collection_free(cc);
    free(table);
}

CTEST(test_lalr_1)
{
    initialize_parser();
    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, lalr_1_cmp, lalr_1_merge);

    U32* table = canonical_collection_generate(cc, precedence_table);
    dump_table(table, cc, token_names, 0);
    canonical_collection_free(cc);
    free(table);
}

CTEST(test_lalr_1_calculator)
{
    const char* lexer_input = "1 + (5 * 9) + 2";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    ValUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 9);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, lalr_1_cmp, lalr_1_merge);

    U32* table = canonical_collection_generate(cc, precedence_table);

    ParserStack* stack = malloc(sizeof(ParserStack) + (sizeof(U32) * 64));
    stack->pos = 0;
    I32 res_idx = parser_parse_lr(&p, stack, table, token_table, value_table, sizeof(ValUnion));

    printf("%s = %lf\n", lexer_input, value_table[res_idx].number);
    assert_double_equal(value_table[res_idx].number, 1 + (5 * 9) + 2, 0.005);

    free(stack);
    canonical_collection_free(cc);
    free(table);
}

CTEST(test_lalr_1_order_of_ops)
{
    const char* lexer_input = "1 + 5 * 9 + 4";
    initialize_parser();

    const char** yyinput = &lexer_input;

    U32 token_table[32];
    ValUnion value_table[32];

    int tok_n = lexer_fill_table(yyinput, &p, token_table, value_table, sizeof(ValUnion), 32);
    assert_int_equal(tok_n, 7);

    CanonicalCollection* cc = canonical_collection_init(&p);
    canonical_collection_resolve(cc, lalr_1_cmp, lalr_1_merge);

    U32* table = canonical_collection_generate(cc, precedence_table);

    ParserStack* stack = malloc(sizeof(ParserStack) + (sizeof(U32) * 64));
    stack->pos = 0;
    I32 res_idx = parser_parse_lr(&p, stack, table, token_table, value_table, sizeof(ValUnion));

    // This parser has no order of ops
    assert_double_equal(value_table[res_idx].number, (((1 + 5) * 9) + 4), 0.005);

    free(stack);
    canonical_collection_free(cc);
    free(table);
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