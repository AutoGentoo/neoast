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

#include "derivation.h"
#include "canonical_collection.h"

namespace parsergen
{
    void LR1::merge_next_lookaheads(const CanonicalCollection* cc, BitVector &dest) const
    {
        bool last_has_empty;
        uint32_t next_item = i + 1;
        do
        {
            // Get the next item
            if (next_item >= derivation->tok_n)
            {
                dest.merge(look_ahead);
                break;
            }

            cc->merge_first_of(dest, derivation->grammar[next_item], last_has_empty);
        } while (last_has_empty);
    }

    void GrammarState::apply_closure()
    {
        // We don't need to recalculate closure
        if (has_closure)
        {
            return;
        }

        // We haven't applied closure to all rules yet
        bool has_dirty = true;

        // These are the rules we have already added to this production
        // We don't need to apply closure multiple times on the same item
        // Wo DO need to merge the lookaheads however
        BitVector expanded_rules(cc->parser()->token_n);

        // Keep applying closure to the dirty rules
        while (has_dirty)
        {
            has_dirty = false;

            for (auto &item: lr1_items)
            {
                // Check if this item needs to be expanded
                // This just involves checking if the next item is a derivation
                // or just an action token
                // We have three options:
                //     1. Next token in this LR(1) is an action -> No action needed
                //     2. Next token is production (expanded) -> Merge lookaheads
                //     3. Next token is production (unexpanded) -> Apply closure by adding all rules describing this grammar

                // No need to apply closure to final items (lookaheads are action tokens)
                if (item.is_final()) continue;
                tok_t curr = item.curr();

                // Option 1 (no action)
                if (cc->is_action(curr)) continue;

                // Check if we have already expanded this token
                if (expanded_rules[cc->to_index(curr)])
                {
                    // Option 2 (merge lookaheads)
                    if (item.has_next())
                    {
                        bool merge_next_lookahead;
                        cc->merge_first_of(item.look_ahead, item.next(), merge_next_lookahead);

                        if (merge_next_lookahead)
                        {
                            // We need to merge in our lookaheads because this item
                            // could be empty and therefore its own lookaheads are not enough

                            // Get the LR(1) lookaheads
                            item.merge_next_lookaheads(cc, item.look_ahead);
                        }
                    }
                    else
                    {
                        // TODO aahh
                    }
                }
                else
                {
                    // Get the LR(1) lookaheads
                    BitVector new_lookaheads(cc->parser()->action_token_n);
                    item.merge_next_lookaheads(cc, new_lookaheads);
                    expanded_rules.select(cc->to_index(curr));

                    // Add all the productions describing this grammar
                    const auto &new_prods = cc->get_productions(curr);
                    has_dirty = true;
                    for (const auto &prod: new_prods)
                    {
                        lr1_items.emplace(prod, 0, new_lookaheads);
                    }
                }
            }
        }

        has_closure = true;
    }

    void GrammarState::resolve() const
    {
        // Check if we are in a resolve cycle
        if (!dfa.empty()) return;

        // Resolving the DFA simply involves finding every
        // possible state transition and creating a resultant
        // state from these transitions. Duplicate state will
        // automatically be consolidated by CanonicalCollection::add_state()

        assert(has_closure && "resolve() called without closure!!");
        std::unordered_map<tok_t, std::vector<LR1>> initial_items;
        for (const auto &lr1: lr1_items)
        {
            if (lr1.is_final()) continue;

            tok_t next = lr1.curr();
            if (initial_items.find(next) == initial_items.end())
            {
                initial_items[next] = {};
            }

            // Place the transitioned LR(1) item into the initial state
            initial_items.at(next).emplace_back(lr1.derivation,
                                                lr1.i + 1,
                                                lr1.look_ahead);
        }

        // Build the DFA
        for (auto &p: initial_items)
        {
            // This will destroy the vectors in initial_items
            // (we don't need them anymore, so it's fine)
            dfa[p.first] = cc->add_state(p.second);
        }

        // Recursively resolve the DFA
        // We need to do this after so that we can consolidate
        // future (depth) states with the ones in here
        // This means breadth first DFA generation
        for (const auto &edge: dfa)
        {
            edge.second->resolve();
        }
    }

    void GrammarState::fill_table(uint32_t* row, uint32_t &rr_conflicts, uint32_t &sr_conflicts,
                                  const uint8_t* precedence_table) const
    {
        // The row starts initialized with zeroes (syntax error)
        // We just need to fill in SHIFT, GOTO (basically just shift), and REDUCE

        // Go through each state transition in fill in SHIFT/GOTO
        for (const auto &tok_and_state: dfa)
        {
            row[cc->to_index(tok_and_state.first)] = cc->get_state_id(tok_and_state.second) | TOK_SHIFT_MASK;
        }

        // Fill any REDUCE moves (if any)
        // REDUCE moves only happen when an LR(1) item is a final item
        for (const auto &lr1: lr1_items)
        {
            // Not REDUCE move
            if (!lr1.is_final()) continue;

            uint32_t action_mask;
            uint32_t reduce_id;

            // There are really two types of REDUCE moves:
            //   REDUCE and ACCEPT
            // The ACCEPT move is marked by the augment rule.
            // The Augment rule will return the augment token
            if (cc->to_index(lr1.derivation->token) == cc->parser()->token_n)
            {
                // ACCEPT rule
                action_mask = TOK_ACCEPT_MASK;
                reduce_id = 0;
            }
            else
            {
                // Simple REDUCE move
                action_mask = TOK_REDUCE_MASK;
                reduce_id = cc->get_reduce_id(lr1.derivation);
            }

            // This is a CLR(1) / LALR(1) grammar, so only place
            // reduce moves in the lookahead slots
            for (const auto &i: lr1.look_ahead)
            {
                // If the row at this point is already filled, there is a conflict
                if (row[i] != TOK_SYNTAX_ERROR)
                {
                    if (row[i] & TOK_REDUCE_MASK)
                    {
                        // TODO
                        rr_conflicts++;
                    }
                    else if (row[i] & TOK_SHIFT_MASK)
                    {
                        // Check if we have a precedence rule
                        if (precedence_table && precedence_table[i] == PRECEDENCE_LEFT)
                        {
                            // Let the table value take reduction
                        }
                        else if (precedence_table && precedence_table[i] == PRECEDENCE_RIGHT)
                        {
                            // Keep the shift
                            continue;
                        }
                        else
                        {
                            // TODO
                            sr_conflicts++;
                        }
                    }
                }

                row[i] = action_mask | reduce_id;
            }
        }
    }
}
