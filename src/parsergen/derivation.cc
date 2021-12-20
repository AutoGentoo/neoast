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

#include <cstring>
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
            next_item++;
        } while (last_has_empty);
    }

    void GrammarState::apply_closure()
    {
        // We don't need to recalculate closure
        if (has_closure) return;

        // These are the rules we have already added to this production
        // We don't need to apply closure multiple times on the same item
        // Wo DO need to merge the lookaheads however
        BitVector expanded_rules(cc->parser()->token_n + 1);

        std::vector<const LR1*> vec1;
        std::vector<const LR1*> vec2;
        for (auto& item : lr1_items)
        {
            vec1.push_back(&item);
        }

        std::vector<const LR1*>* curr_dirty = &vec1;
        std::vector<const LR1*>* next_dirty = &vec2;

        // Keep applying closure to the dirty rules
        while (!curr_dirty->empty())
        {
            for (auto* item: *curr_dirty)
            {
                // Check if this item needs to be expanded
                // This just involves checking if the next item is a derivation
                // or just an action token
                // We have three options:
                //     1. Next token in this LR(1) is an action -> No action needed
                //     2. Next token is production (expanded) -> Merge lookaheads
                //     3. Next token is production (unexpanded) -> Apply closure by adding all rules describing this grammar

                // No need to apply closure to final items (lookaheads are action tokens)
                if (item->is_final()) continue;
                tok_t curr = item->curr();

                // Option 1 (no action)
                if (cc->is_action(curr)) continue;

                // Check if we have already expanded this token
                if (expanded_rules[cc->to_index(curr)])
                {
                    // Option 2 (merge lookaheads)
                    // We need to merge the lookahead with every other
                    // rule that matches this token
                    BitVector new_lookaheads(cc->parser()->action_token_n);
                    item->merge_next_lookaheads(cc, new_lookaheads);
                    for (auto& item2 : lr1_items)
                    {
                        if (item2.derivation->token == curr)
                        {
                            item2.look_ahead.merge(new_lookaheads);
                        }
                    }
                }
                else
                {
                    // Get the LR(1) lookaheads
                    BitVector new_lookaheads(cc->parser()->action_token_n);
                    item->merge_next_lookaheads(cc, new_lookaheads);
                    expanded_rules.select(cc->to_index(curr));

                    // Add all the productions describing this grammar
                    const auto &new_prods = cc->get_productions(curr);
                    for (const auto &prod: new_prods)
                    {
                        auto p = lr1_items.emplace(prod, 0, new_lookaheads);
                        next_dirty->push_back(&*p.first);
                    }
                }
            }

            curr_dirty->clear();
            std::vector<const LR1*>* temp = next_dirty;
            next_dirty = curr_dirty;
            curr_dirty = temp;
        }

        // Keep updating the lookaheads until nothing changes
        bool dirty = true;
        while (dirty)
        {
            dirty = false;
            for (const auto& item : lr1_items)
            {
                if (item.is_final() || cc->is_action(item.curr())) continue;
                assert(expanded_rules[cc->to_index(item.curr())]);

                // Option 2 (merge lookaheads)
                // We need to merge the lookahead with every other
                // rule that matches this token
                BitVector new_lookaheads(cc->parser()->action_token_n);
                item.merge_next_lookaheads(cc, new_lookaheads);
                for (auto& item2 : lr1_items)
                {
                    if (item2.derivation->token == item.curr())
                    {
                        dirty = dirty || item2.look_ahead.merge(new_lookaheads);
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
        memset(row, 0, sizeof(uint32_t) * cc->parser()->token_n);

        // Go through each state transition in fill in SHIFT/GOTO
        for (const auto &tok_and_state: dfa)
        {
            uint32_t target_state = tok_and_state.second->get_id();
            assert(target_state < cc->size());
            row[cc->to_index(tok_and_state.first)] = target_state | TOK_SHIFT_MASK;
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
                            fprintf(stderr, "SR conflict tok %d state %d attempting to shift to %d\n",
                                    i, get_id(),
                                    row[i] & TOK_MASK);
                            sr_conflicts++;
                        }
                    }
                }

                row[i] = action_mask | reduce_id;
            }
        }
    }

    bool GrammarState::lalr_equal(const GrammarState &other) const
    {
        return parsergen::lalr_equal(lr1_items, other.lr1_items);
    }

    void GrammarState::lalr_merge(const GrammarState* other) const
    {
        // TODO(tumbar) Find a way to keep this set from lalr_equal
        std::unordered_set<const LR1*, HasherPtr<const LR1*>, EqualizerPtr<const LR0*>> build;

        for (const auto& ours : lr1_items)
        {
            auto p = build.insert(&ours);
            assert(p.second);
        }

        // Merge all redundant state
        for (const auto& theirs : other->lr1_items)
        {
            auto p = build.find(&theirs);
            assert(p != build.end());

            // We can merge this LR(1) item down
            (*p)->lalr_merge(theirs);
        }

        // All LR(1) items are now merged together
        // Keep track of states that are merged
        merged_states.push_back(other);
    }

    bool lalr_equal(const std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> &a,
                    const std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> &b)
    {
        if (a.size() != b.size()) return false;

        // Perform LR0 comparison on every LR1 item
        // We can make this easier by generating another set with LR0 predicate
        std::unordered_set<const LR1*, HasherPtr<const LR1*>, EqualizerPtr<const LR0*>> build;

        for (const auto& a_i : a)
        {
            auto p = build.insert(&a_i);
            assert(p.second);
        }

        // Try to add each item from b
        // If any add succeeds, this item does not exist in a
        for (const auto& b_i : b)
        {
            auto p = build.insert(&b_i);
            if (p.second) return false;
        }

        return true;
    }
}
