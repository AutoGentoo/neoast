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
#include <ostream>
#include <sstream>
#include "util.h"

void dump_item(const LR_1* lr1, uint32_t tok_n, const char* tok_names, std::ostream &os)
{
    uint8_t printed = 0;
    os << "['";
    for (uint32_t i = 0; i < lr1->grammar->tok_n; i++)
    {
        if (i == lr1->item_i && !printed)
        {
            printed = 1;
            os << "•";
        }

        os << tok_names[
                (lr1->grammar->grammar[i] < NEOAST_ASCII_MAX)
                ? lr1->grammar->grammar[i] : lr1->grammar->grammar[i] - NEOAST_ASCII_MAX];
    }

    if (lr1->final_item)
        os << "•";

    os << "'";

    if (lr1->final_item)
        os << "!";

    os << ",";

    // Print the lookaheads
    printed = 0;
    for (uint32_t i = 0; i < tok_n; i++)
    {
        if (lr1->look_ahead[i])
        {
            if (printed)
            {
                os << "/";
            }

            os << tok_names[i];
            printed = 1;
        }
    }
    os << "];";
}

void dump_state(const GrammarState* state, uint32_t tok_n, uint32_t lookahead_n, const char* tok_names,
                uint8_t line_wrap, std::ostream &os)
{
    uint8_t is_first = 1;
    for (const LR_1* item = state->head_item; item; item = item->next)
    {
        if (!is_first && line_wrap)
        {
            // Print spacing
            os << std::string(12 + (6 * tok_n), ' ');
        }

        is_first = 0;
        dump_item(item, lookahead_n, tok_names, os);

        if (line_wrap)
        {
            os << '\n';
        }
        else if (item->next)
            os << ' ';
    }
}

void dump_table_cxx(
        const uint32_t* table, const CanonicalCollection* cc,
        const char* tok_names, uint8_t state_wrap,
        std::ostream& os, const char* indent_str)

{
    if (!indent_str)
        indent_str = "";

    uint32_t i = 0;
    os << indent_str << "token_id: ";
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            os << "|";
        os << variadic_string(" %*d  |", 2, col);
    }

    os << "\n" << indent_str << "token    : ";
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            os << "|";
        os << variadic_string("  %c  |", tok_names[col]);
    }
    os << "\n" << indent_str << "___________";
    for (uint32_t col = 0; col < cc->parser->token_n; col++)
    {
        if (col == cc->parser->action_token_n)
            os << "_";
        os << "______";
    }
    os << "\n";

    for (uint32_t row = 0; row < cc->state_n; row++)
    {
        os << indent_str << variadic_string("state %02d : ", row);
        for (uint32_t col = 0; col < cc->parser->token_n; col++, i++)
        {
            if (col == cc->parser->action_token_n)
                os << "|";

            if (!table[i])
                os << "  E  |";
            else if (table[i] & TOK_ACCEPT_MASK)
                os << "  A  |";
            else if (table[i] & TOK_SHIFT_MASK && col < cc->parser->action_token_n)
                os << variadic_string(" S%02d |", table[i] & TOK_MASK);
            else if (table[i] & TOK_SHIFT_MASK)
                os << variadic_string(" G%02d |", table[i] & TOK_MASK);
            else if (table[i] & TOK_REDUCE_MASK)
                os << variadic_string(" R%02d |", table[i] & TOK_MASK);
        }

        dump_state(cc->all_states[row], cc->parser->token_n, cc->parser->action_token_n, tok_names, state_wrap, os);

        if (!state_wrap)
            os << "\n";
    }
}

void dump_table(const uint32_t* table,
                const CanonicalCollection* cc,
                const char* tok_names,
                uint8_t state_wrap,
                FILE* fp,
                const char* indent_str)
{
    std::ostringstream os_fp;

    dump_table_cxx(table, cc, tok_names, state_wrap, os_fp, indent_str);
    std::string s = os_fp.str();

    fwrite(s.c_str(),s.length(), 1, fp);
}
