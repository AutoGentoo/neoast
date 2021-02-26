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

#include <parsergen/canonical_collection.h>

void dump_item(const LR_1* lr1, uint32_t tok_n, const char* tok_names, FILE* fp)
{
    uint8_t printed = 0;
    fprintf(fp, "['");
    for (uint32_t i = 0; i < lr1->grammar->tok_n; i++)
    {
        if (i == lr1->item_i && !printed)
        {
            printed = 1;
            fprintf(fp, "•");
        }

        fprintf(fp, "%c", tok_names[lr1->grammar->grammar[i] < NEOAST_ASCII_MAX ? lr1->grammar->grammar[i] : lr1->grammar->grammar[i] - NEOAST_ASCII_MAX]);
    }

    if (lr1->final_item)
        fprintf(fp, "•");

    fprintf(fp, "'");

    if (lr1->final_item)
        fprintf(fp, "!");

    fprintf(fp, ",");

    // Print the lookaheads
    printed = 0;
    for (uint32_t i = 0; i < tok_n; i++)
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

void dump_state(const GrammarState* state, uint32_t tok_n, uint32_t lookahead_n, const char* tok_names, uint8_t line_wrap, FILE* fp)
{
    uint8_t is_first = 1;
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

void dump_table(const uint32_t* table,
                const CanonicalCollection* cc,
                const char* tok_names,
                uint8_t state_wrap,
                FILE* fp,
                const char* indent_str)
{
    if (!indent_str)
        indent_str = "";

    uint32_t i = 0;
    fprintf(fp, "%stoken_id : ", indent_str);
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "|");
        fprintf(fp, " %*d  |", 2, col);
    }
    fprintf(fp, "\n");

    fprintf(fp, "%stoken    : ", indent_str);
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "|");
        fprintf(fp, "  %c  |", tok_names[col]);
    }
    fprintf(fp, "\n");
    fprintf(fp, "%s___________", indent_str);
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            fprintf(fp, "_");
        fprintf(fp, "______");
    }
    fprintf(fp, "\n");

    for (uint32_t row = 0; row < cc->state_n; row++)
    {
        fprintf(fp, "%sstate %02d : ", indent_str, row);
        for (uint32_t col = 0; col < cc->parser->token_n; col++, i++)
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