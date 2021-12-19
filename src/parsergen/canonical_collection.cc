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
    : debug_info_(debug_info), parser_(parser), dfa_(nullptr), state_n_(0)
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
        auto p = states_.emplace(std::make_shared<GrammarState>(this, initial_vector, state_n_));
        if (p.second)
        {
            // This is a new (unseen) state in the DFA
            // We need to register this new state number
            state_id_to_ptr_[state_n_++] = p.first->get();
        }

        return p.first->get();
    }

    struct LALR1Equal
    {
        bool operator()(
                const sp<GrammarState>& a,
                const sp<GrammarState>& b) const
        { return a->lalr_equal(*b); }
    };

    void CanonicalCollection::resolve(parser_t type)
    {
        // Add the augment rule
        BitVector augment_lookahead(parser_->action_token_n);
        augment_lookahead.select(0); // select EOF
        std::vector<LR1> augment_vector{LR1(&parser_->grammar_rules[0], 0, augment_lookahead)};

        // Head of DFA is the augment state
        dfa_ = add_state(augment_vector);
        dfa_->resolve(); // resolve the entire DFA

        if (type == CLR_1)
        {
            // CLR(1) tables need a canonical collection of LR(1)
            // items. This is what the DFA is currently.
            return;
        }

        // LALR(1) does not distinguish between LR(1) with different lookaheads
        // To consolidate these we can build an unordered_set of GrammarStates
        // that follows a different equality predicate (only compare LR(0) items)
        std::unordered_set<
                sp<GrammarState>, HasherPtr<sp<GrammarState>>,
                LALR1Equal> lalr_states;

        for (const auto& clr_state : states_)
        {
            auto p = lalr_states.insert(clr_state);
            if (!p.second)
            {
                // Choose the state with the lower id number
                const GrammarState* keep, *merge;
                if ((*p.first)->get_id() < clr_state->get_id())
                {
                    keep = p.first->get();
                    merge = clr_state.get();
                }
                else
                {
                    keep = clr_state.get();
                    merge = p.first->get();
                }

                // We can merge these two states together
                keep->lalr_merge(merge);

                // Move the last ID into the removed slot
                // Delete the last ID
                uint32_t to_remove_id = --state_n_;
                const GrammarState* to_remove = state_id_to_ptr_.at(to_remove_id);
                assert(to_remove->get_id() == to_remove_id);

                if (merge->get_id() != to_remove_id)
                {
                    state_id_to_ptr_[merge->get_id()] = to_remove;
                    to_remove->set_id(merge->get_id()); // merge the highest id into the open slot
                }
                state_id_to_ptr_.erase(to_remove_id);
                merge->set_id(keep->get_id()); // merge the state into the lower id
                keep->set_id(keep->get_id()); // set ids for all states merged into this one
            }
        }
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
