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

#ifndef NEOAST_DERIVATION_H
#define NEOAST_DERIVATION_H

#include <neoast.h>
#include "c_pub.h"
#include "cc_common.h"

#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include <unordered_set>

namespace parsergen
{
    template<typename H>
    struct Hasher
    {
        std::size_t operator()(const H& k) const
        { return k.hash(); }
    };
    template<typename H>
    struct HasherPtr
    {
        std::size_t operator()(const H& k) const
        { return k->hash(); }
    };
    template<typename H>
    struct HasherRawPtr
    {
        std::size_t operator()(const H* k) const
        { return (size_t)k; }
    };
    template<typename H>
    struct Equalizer
    {
        bool operator()(const H& k, const H& l) const
        { return k == l; }
    };
    template<typename H>
    struct EqualizerPtr
    {
        bool operator()(const H& k, const H& l) const
        { return *k == *l; }
    };

    struct CanonicalCollection;
    struct LR0;
    struct LR1;
    struct GrammarState;

    struct LR0
    {
        const GrammarRule* derivation;      //!< Grammar derivation we are pointing to
        uint32_t i;                         //!< Index where we are in the current grammar rule

        LR0(const GrammarRule* derivation, uint32_t item_i)
                : derivation(derivation), i(item_i)
        {
        }

        inline uint32_t n() const { return derivation->tok_n; }

        inline bool is_final() const { return i == derivation->tok_n; }
        inline bool operator==(const LR0& other) const
        {
            return derivation == other.derivation &&
                   i == other.i;
        }
        inline bool operator<(const LR0& other) const
        {
            return (derivation == other.derivation) ? i < other.i : derivation < other.derivation;
        }

        inline tok_t curr() const { return derivation->grammar[i]; }
        inline tok_t next() const { return derivation->grammar[i + 1]; }
        inline bool has_next() const { return i + 1 < n(); }
        inline size_t hash() const { return ((size_t) derivation) + i; }
    };

    struct LR1 : public LR0
    {
        /**
         * An LR(1) item is an LR(0) item + lookahead
         */
        // mutable because it should not be used for sorting
        mutable BitVector look_ahead;    //!< LR(1) lookaheads

        LR1(const GrammarRule* derivation, uint32_t item_i,
            BitVector lookaheads)
        : LR0(derivation, item_i), look_ahead(std::move(lookaheads))
        {
        }

        bool operator==(const LR1& other) const
        {
            return LR0::operator==(other) && look_ahead == other.look_ahead;
        }

        void lalr_merge(const LR1& other) const
        {
            look_ahead.merge(other.look_ahead);
        }

        /**
         * Get the first_of the next item in the LR(1) item
         * @param cc global canonical collection keeping track of basic first_of
         * @param dest destination bitvector to store first_ofs
         */
        void merge_next_lookaheads(const CanonicalCollection* cc,
                                   parsergen::BitVector& dest) const;
    };

    /**
     * Compare two LR1 sets using LR0 comparison to
     * consolidate an LR1 canonical collection to LR0 for
     * LALR(1) table generation
     * @param a first set
     * @param b second set
     * @return true if equivalent, false if not
     */
    bool lalr_equal(
            const std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>>& a,
            const std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>>& b);

    struct GrammarState
    {
    private:
        /**
         * A collection of derivations that will keep track of where the parser
         * should go to upon the input of a certain token
         */
        bool has_closure;
        CanonicalCollection* cc;
        std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> lr1_items;   //!< Items in the state
        mutable std::vector<const GrammarState*> merged_states;
        mutable std::unordered_map<tok_t, const GrammarState*> dfa;       //!< State transitions
        mutable uint32_t state_id;

        /**
         * Given the initialization rules are already added to the state,
         * Apply LR closure to this state.
         */
        void apply_closure();

    public:
        GrammarState(GrammarState&&) = delete;

        GrammarState(CanonicalCollection* cc,
                     const std::vector<LR1>& initial_items,
                     uint32_t state_id_)
        : cc(cc), has_closure(false), state_id(state_id_)
        {
            for (const auto& i : initial_items)
            {
                lr1_items.insert(i);
            }

            apply_closure();
        }

        std::unordered_set<LR1>::const_iterator begin() const { return lr1_items.begin(); }
        std::unordered_set<LR1>::const_iterator end() const { return lr1_items.end(); }

        std::unordered_map<tok_t, const GrammarState*>::const_iterator dfa_begin() const { return dfa.begin(); };
        std::unordered_map<tok_t, const GrammarState*>::const_iterator dfa_end() const { return dfa.end(); };

        /**
         * Resolve all state transitions out of this state
         * (fill dfa)
         */
        void resolve() const;
        void fill_table(uint32_t row[],
                        uint32_t& rr_conflicts,
                        uint32_t& sr_conflicts,
                        const uint8_t* precedence_table) const;

        bool operator==(const GrammarState& other) const
        {
            return lr1_items == other.lr1_items;
        }

        void set_id(uint32_t id) const
        {
            state_id = id;
            for (const auto& i : merged_states)
            {
                i->set_id(id);
            }
        }
        uint32_t get_id() const { return state_id; }

        bool lalr_equal(const GrammarState& other) const;

        void lalr_merge(const GrammarState* other) const;

        inline size_t hash() const
        {
            size_t out = 0;
            for (const auto& i : lr1_items)
                out += i.hash();
            return out;
        }
    };
}

#undef NEOAST_DEFINE_HASH

#endif //NEOAST_DERIVATION_H
