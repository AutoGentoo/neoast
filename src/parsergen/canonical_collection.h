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


#ifndef NEOAST_CANONICAL_COLLECTION_H
#define NEOAST_CANONICAL_COLLECTION_H

#include "neoast.h"
#include "c_pub.h"
#include "bit_vector.h"
#include "derivation.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <common/context.h>

// Codegen

namespace parsergen
{
    template<typename T> using sp = std::shared_ptr<T>;
    struct CanonicalCollection
    {
    private:
        enum FirstOfOption
        {
            NONE,
            INITIALIZED = 1,
            HAS_EMPTY = 2,
        };

        const GrammarParser* parser_;               //!< Parent parser with grammar rules
        Context* context_;                          //!< Used for emitting errors and warnings
        const TokenPosition* const* reduce_positions_;  //!< Positions of reduce rules in input file
        const GrammarState* dfa_;                   //!< Head of the LR DFA
        uint32_t state_n_;

        /**
         * Whenever a new state is added to the DFA,
         * it must register with the entire canonical collection
         * so that we can avoid redundant states.
         */
        std::unordered_set<sp<GrammarState>, HasherPtr<sp<GrammarState>>, EqualizerPtr<sp<GrammarState>>> states_;

        /**
         * Maps each grammar state to some id number
         * And the ID number back to the state
         */
        std::unordered_map<uint32_t, const GrammarState*> state_id_to_ptr_;

        /**
         * Reduction IDs are simply indices in the grammar rule table
         * We need to agree on rule indices before hand
         */
        std::unordered_map<const GrammarRule*, uint32_t, HasherRawPtr<const GrammarRule>> reduce_ids_;
        std::unordered_map<tok_t, std::vector<const GrammarRule*>> productions_;

        /**
         * Caching the first_of vectors will help speed of DFA resolution
         */
        std::unordered_map<tok_t, BitVector> first_ofs_;
        std::unordered_map<tok_t, int> first_options_;

        void lr_1_firstof_impl(BitVector& dest,
                               tok_t grammar_id,
                               BitVector& visiting);
        void lr_1_firstof_init(tok_t grammar_id);

    public:
        CanonicalCollection(const GrammarParser* parser, Context* context,
                            const TokenPosition* const* reduce_positions);

        inline const GrammarParser* parser() const { return parser_; }
        inline Context* context() { return context_; }

        const GrammarState* add_state(const std::vector<LR1>& initial_vector);

        inline uint32_t size() const { return state_n_; }
        inline const GrammarState* get_state(uint32_t id) const { return state_id_to_ptr_.at(id); }

        inline uint32_t get_reduce_id(const GrammarRule* rule) const
        {
            return reduce_ids_.at(rule);
        }

        inline const TokenPosition* get_position(const GrammarRule* rule) const
        { return get_position(get_reduce_id(rule)); }

        inline const TokenPosition* get_position(uint32_t reduce_id) const
        { return reduce_positions_[reduce_id]; }

        inline tok_t to_index(tok_t token) const
        {
            if (token < NEOAST_ASCII_MAX)
            {
                assert(parser_->ascii_mappings);
                assert(parser_->ascii_mappings[token] >= NEOAST_ASCII_MAX);
                token = parser_->ascii_mappings[token];
            }

            return token - NEOAST_ASCII_MAX;
        }

        inline bool is_action(tok_t token) const
        {
            return to_index(token) < parser_->action_token_n;
        }

        inline void merge_first_of(BitVector& dest,
                                   tok_t token, bool& has_empty) const
        {
            if (is_action(token))
            {
                dest.select(to_index(token));
                has_empty = false;
                return;
            }

            dest.merge(first_ofs_.at(token));
            has_empty = first_options_.at(token) & HAS_EMPTY;
        }

        inline const std::vector<const GrammarRule*>& get_productions(tok_t grammar) const
        {
            return productions_.at(grammar);
        }

        inline std::unordered_map<tok_t, std::vector<const GrammarRule*>>::const_iterator
        begin_productions() const { return productions_.begin(); }

        inline std::unordered_map<tok_t, std::vector<const GrammarRule*>>::const_iterator
        end_productions() const { return productions_.end(); }

        /**
         * @return Number of items in the parsing table
         */
        inline size_t table_size() const { return parser_->token_n * size(); }

        /**
         * Resolve the entire DFA by applying closures and
         * state transitions recursively
         */
        void resolve(parser_t type);

        /**
         * Write the parsing table to a matrix
         * @param table matrix to fill parsing table with
         * @param precedence_table precedence_table  or null
         * @return error code or 0 for success
         */
        uint32_t generate(uint32_t* table, const uint8_t* precedence_table) const;
    };
}

#endif //NEOAST_CANONICAL_COLLECTION_H
