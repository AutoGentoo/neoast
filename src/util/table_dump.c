#include <lexer.h>
#include <parser.h>
#include <parsergen/canonical_collection.h>

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

void dump_state(const GrammarState* state, U32 tok_n, U32 lookahead_n, const char* tok_names, U8 line_wrap)
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
        dump_item(item, lookahead_n, tok_names);

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
            else if (table[i] & TOK_SHIFT_MASK && col < cc->parser->action_token_n)
            {
                printf(" S%02d |", table[i] & TOK_MASK);
            }
            else if (table[i] & TOK_SHIFT_MASK)
            {
                printf(" G%02d |", table[i] & TOK_MASK);
            }
            else if (table[i] & TOK_REDUCE_MASK)
            {
                printf(" R%02d |", table[i] & TOK_MASK);
            }
        }

        dump_state(cc->all_states[row], cc->parser->token_n, cc->parser->action_token_n, tok_names, state_wrap);

        if (!state_wrap)
            printf("\n");
    }
}
