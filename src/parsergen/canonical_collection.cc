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


#include <util/util.h>
#include "canonical_collection.h"
#include "derivation.h"

namespace parsergen
{
    CanonicalCollection::CanonicalCollection(const GrammarParser* parser, const DebugInfo* debug_info)
    : debug_info_(debug_info), parser_(parser), dfa_(nullptr)
    {
        // Initialize the grammar productions to an O(1) map
        for (uint32_t i = 0; i < parser_->grammar_n; i++)
        {
            const GrammarRule* rule = &parser_->grammar_rules[i];
            reduce_ids_[rule] = i;
            if (productions_.find(rule->token) == productions_.end())
            {
                // Add a new empty entry
                productions_[rule->token] = {};

                // Initialize the first_of vectors
                // Clear everyone at the start
                first_ofs_.emplace(rule->token, parser_->action_token_n);
                first_options_.emplace(rule->token, FirstOfOption::NONE);
            }

            // Add the rule to the production
            productions_.at(rule->token).push_back(rule);
        }

        // Fill the first_of vectors
        for (const auto &i: first_options_)
        {
            if (!(i.second & FirstOfOption::INITIALIZED))
            {
                lr_1_firstof_init(i.first);
            }
        }
    }

    void CanonicalCollection::lr_1_firstof_impl(
            parsergen::BitVector &dest,
            tok_t grammar_id,
            parsergen::BitVector &visiting)
    {
        // Don't try to recursively expand the same rules
        if (visiting[to_index(grammar_id)])
        {
            // This will avoid cycles
            return;
        }

        // Short circuit if we already initialized this vector
        if (first_options_.at(grammar_id) & FirstOfOption::INITIALIZED)
        {
            dest.merge(first_ofs_.at(grammar_id));
            return;
        }

        visiting.select(to_index(grammar_id));

        for (const auto* production: productions_.at(grammar_id))
        {
            if (production->tok_n == 0)
            {
                first_options_[grammar_id] |= FirstOfOption::HAS_EMPTY;
            }
            else
            {
                tok_t first_tok = production->grammar[0];

                if (is_action(first_tok))
                {
                    // Simple case
                    dest.select(to_index(first_tok));
                }
                else
                {
                    if (visiting[to_index(first_tok)])
                    {
                        // Recursively resolve the first_of vector
                        lr_1_firstof_impl(first_ofs_.at(first_tok),
                                          first_tok, visiting);

                        // If our rule only contains a rule that could be empty,
                        // we also could be empty!
                        if (first_options_.at(first_tok) & FirstOfOption::HAS_EMPTY &&
                            production->tok_n == 1)
                        {
                            first_options_[grammar_id] |= FirstOfOption::HAS_EMPTY;
                        }

                        // We can now merge the initialized bit vectors together
                        dest.merge(first_ofs_.at(first_tok));
                    }
                }
            }
        }

        visiting.clear(to_index(grammar_id));
        first_options_[grammar_id] |= FirstOfOption::INITIALIZED;
    }

    void CanonicalCollection::lr_1_firstof_init(tok_t grammar_id)
    {
        parsergen::BitVector already_visited(parser_->token_n + 1);
        lr_1_firstof_impl(first_ofs_.at(grammar_id), grammar_id, already_visited);
    }

    const GrammarState* CanonicalCollection::add_state(const std::vector<LR1>& initial_vector)
    {
        std::shared_ptr<GrammarState> state(new GrammarState (this, initial_vector));
        state->apply_closure();
        const auto& i = states_.find(state);
        if (i == states_.end())
        {
            // This is a new (unseen) state in the DFA
            // We can add it to the set
            uint32_t new_state_id = state_ids_.size();
            auto out_p = states_.insert(state);
            if (!out_p.second)
                throw Exception("Failed to add state to canonical collection");

            state_ids_[out_p.first->get()] = new_state_id;
            state_id_to_ptr_[new_state_id] = out_p.first->get();
            return out_p.first->get();
        }
        else
        {
            return i->get();
        }
    }

    void CanonicalCollection::resolve()
    {
        // Add the augment rule
        // TODO(tumbar) IS THE FIRST RULE ALWAYS THE AUGMENT RULE?
        BitVector augment_lookahead(parser_->action_token_n);
        augment_lookahead.select(0); // select EOF
        std::vector<LR1> augment_vector{LR1(&parser_->grammar_rules[0], 0, augment_lookahead)};

        // Head of DFA is the augment state
        dfa_ = add_state(augment_vector);
        dfa_->resolve(); // resolve the entire DFA

        // TODO(tumbar) Consolidate for LALR(1) CC
    }

    uint32_t* CanonicalCollection::generate(const uint8_t* precedence_table, uint8_t* error) const
    {
        auto* table = static_cast<uint32_t*>(calloc(parser_->token_n * states_.size(), sizeof(uint32_t)));
        auto* curr_row = table;
        uint32_t rr_conflicts = 0, sr_conflicts = 0;

        for (uint32_t state_id = 0; state_id < size(); state_id++)
        {
            auto* state = get_state(state_id);

            // Fill the row
            state->fill_table(curr_row, rr_conflicts, sr_conflicts, precedence_table);

            // Increment by an entire row
            curr_row += parser_->token_n;
        }

        *error = rr_conflicts || sr_conflicts;
        return table;
    }
}