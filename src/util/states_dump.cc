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


#include "util.h"
#define RULE_ID_WIDTH (4)

static void dump_state(
        const parsergen::CanonicalCollection* cc,
        const parsergen::GrammarState* state,
        std::ostream& os)
{
    os << "\nState " << state->get_id() << "\n\n";
    std::vector<const parsergen::LR1*> reduce_items;
    for (const auto& lr1 : *state)
    {
        auto s = os.tellp();
        os << variadic_string("   % *d ", RULE_ID_WIDTH, cc->get_reduce_id(lr1.derivation))
           << cc->parser()->token_names[cc->to_index(lr1.derivation->token)] << " →";
        for (uint32_t grammar_i = 0; grammar_i < lr1.derivation->tok_n; grammar_i++)
        {
            if (grammar_i == lr1.i) os << " •";
            os << " " << cc->parser()->token_names[cc->to_index(lr1.derivation->grammar[grammar_i])];
        }
        if (lr1.is_final())
        {
            reduce_items.push_back(&lr1);
            os << " •";
        }
        size_t len = os.tellp() - s;
        os << std::string(70 - len, ' ');
        for (const auto& la : lr1.look_ahead)
        {
            os << " " << cc->parser()->token_names[la];
        }
        os << "\n";
    }

    os << "\n";
    // Dump the DFA state transitions
    for (auto dfa_iter = state->dfa_begin();
         dfa_iter != state->dfa_end(); ++dfa_iter)
    {
        os << "   " << cc->parser()->token_names[cc->to_index(dfa_iter->first)]
           << " → " << dfa_iter->second->get_id() << "\n";
    }
}

void dump_states_cxx(
        const parsergen::CanonicalCollection* cc,
        std::ostream& os)
{
    // Dump the grammar information
    os << "Grammar\n";
    for (auto iter = cc->begin_productions(); iter != cc->end_productions(); iter++)
    {
        os << "\n";
        const auto& grammar_unit = *iter;
        uint32_t indent_len = 0;
        for (const auto& production : grammar_unit.second)
        {
            os << variadic_string("   % *d ", RULE_ID_WIDTH, cc->get_reduce_id(production));
            if (indent_len == 0)
            {
                std::string grammar_name = cc->parser()->token_names[cc->to_index(production->token)];
                indent_len = grammar_name.size();
                os << grammar_name << ":";
            }
            else
            {
                os << std::string(indent_len, ' ') << "|";
            }

            for (uint32_t grammar_i = 0; grammar_i < production->tok_n; grammar_i++)
            {
                os << " " << cc->parser()->token_names[cc->to_index(production->grammar[grammar_i])];
            }
            if (!production->tok_n) os << " ε";
            os << "\n";
        }
    }

    for (uint32_t i = 0; i < cc->size(); i++)
    {
        dump_state(cc, cc->get_state(i), os);
    }
}
