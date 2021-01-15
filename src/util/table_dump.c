#include <lexer.h>
#include <parsergen/canonical_collection.h>

void dump_item(const LR_1* lr1, U32 tok_n, const char* tok_names, FILE* fp)
{
    U8 printed = 0;
    fprintf(fp, "['");
    for (U32 i = 0; i < lr1->grammar->tok_n; i++)
    {
        if (i == lr1->item_i && !printed)
        {
            printed = 1;
            fprintf(fp, "•");
        }

        fprintf(fp, "%c", tok_names[lr1->grammar->grammar[i]]);
    }

    if (lr1->final_item)
        fprintf(fp, "•");

    fprintf(fp, "'");

    if (lr1->final_item)
        fprintf(fp, "!");

    fprintf(fp, ",");

    // Print the lookaheads
    printed = 0;
    for (U32 i = 0; i < tok_n; i++)
    {
        if (lr1->look_ahead[i])
        {
            if (printed)
            {
                fprintf(fp, "/");
            }

            fprintf(fp, "%c", tok_names[i]);
            printed = 1;
        }
    }
    fprintf(fp, "];");
}

void dump_state(const GrammarState* state, U32 tok_n, U32 lookahead_n, const char* tok_names, U8 line_wrap, FILE* fp)
{
    U8 is_first = 1;
    for (const LR_1* item = state->head_item; item; item = item->next)
    {
        if (!is_first && line_wrap)
        {
            // Print spacing
            fprintf(fp, "%*c", 12 + (6 * tok_n), ' ');
        }

        is_first = 0;
        dump_item(item, lookahead_n, tok_names, fp);

        if (line_wrap)
        {
            fputc('\n', fp);
        }
        else if (item->next)
            fputc(' ', fp);
    }
}

void dump_table(const U32* table,
                const CanonicalCollection* cc,
                const char* tok_names,
                U8 state_wrap,
                FILE* fp,
                const char* indent_str)
{
    if (!indent_str)
        indent_str = "";

    U32 i = 0;
    fprintf(fp, "%stoken_id : ", indent_str);
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "|");
        fprintf(fp, " %*d  |", 2, col);
    }
    fprintf(fp, "\n");

    fprintf(fp, "%stoken    : ", indent_str);
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "|");
        fprintf(fp, "  %c  |", tok_names[col]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "%s___________", indent_str);
    for (U32 col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "_");
        fprintf(fp, "______");
    }
    fprintf(fp, "\n");

    for (U32 row = 0; row < cc->state_n; row++)
    {
        fprintf(fp, "%sstate %02d : ", indent_str, row);
        for (U32 col = 0; col < cc->parser->token_n; col++, i++)
        {
            if (col == cc->parser->action_token_n)
                fprintf(fp, "|");

            if (!table[i])
                fprintf(fp, "  E  |");
            else if (table[i] & TOK_ACCEPT_MASK)
                fprintf(fp, "  A  |");
            else if (table[i] & TOK_SHIFT_MASK && col < cc->parser->action_token_n)
            {
                fprintf(fp, " S%02d |", table[i] & TOK_MASK);
            }
            else if (table[i] & TOK_SHIFT_MASK)
            {
                fprintf(fp, " G%02d |", table[i] & TOK_MASK);
            }
            else if (table[i] & TOK_REDUCE_MASK)
            {
                fprintf(fp, " R%02d |", table[i] & TOK_MASK);
            }
        }

        dump_state(cc->all_states[row], cc->parser->token_n, cc->parser->action_token_n, tok_names, state_wrap, fp);

        if (!state_wrap)
            fprintf(fp, "\n");
    }
}
