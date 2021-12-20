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

#ifndef NEOAST_CC_COMMON_H
#define NEOAST_CC_COMMON_H

#include <vector>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <memory>


namespace parsergen
{
    template<typename T> using sp = std::shared_ptr<T>;

    class BitVector
    {
        uint32_t len_;
        std::vector<uint32_t> v;
    public:
        class iterator
        {
            const BitVector& parent;
            uint32_t i;

        public:
            explicit iterator(const BitVector& parent, uint32_t i_)
            : i(i_), parent(parent)
            {
                // Go to the first filled slot
                for (; i < parent.size() && !parent[i]; i++);
            }

            inline void operator++()
            {
                for (i++; i < parent.size() && !parent[i]; i++);
            }

            inline uint32_t operator*() const { return i; }
            inline bool operator!=(const iterator& other) const { return i != other.i; }
        };

        BitVector() : len_(0) {};

        explicit BitVector(uint32_t reserve_n) : len_(0)
        {
            grow(reserve_n);
        };

        inline iterator begin() const { return iterator(*this, 0); }
        inline iterator end() const { return iterator(*this, size()); }

        inline void grow(uint32_t growth_amt)
        {
            len_ += growth_amt;
            uint32_t n = (len_ / 32) + 1;
            if (n <= v.size()) return; // nothing to do here
            v.reserve(n);
            for (uint32_t i = v.size(); i < n; i++) v.push_back(0);
        }

        inline size_t size() const { return len_; }

        inline bool has_any() const
        {
            return std::any_of(v.begin(), v.end(),
                        [](uint32_t i)
                        { return i; }
            );
        }

        inline bool has_none() const
        {
            return std::none_of(v.begin(), v.end(),
                        [](uint32_t i)
                        { return i; }
            );
        }

        inline void clear()
        {
            for (auto &i: v)
            { i = 0; }
        }

        inline bool operator[](uint32_t action_tok) const
        {
            assert(action_tok < len_);
            return v[action_tok / 32] & (1 << (action_tok % 32));
        }

        inline void select(uint32_t action_tok)
        {
            assert(action_tok < len_);
            v[action_tok / 32] |= (1 << (action_tok % 32));
        }

        inline void clear(uint32_t action_tok)
        {
            assert(action_tok < len_);
            v[action_tok / 32] &= ~(1 << (action_tok % 32));
        }

        inline bool merge(const BitVector &r)
        {
            uint32_t i = 0;
            bool changed = false;
            for (auto iter: r.v)
            {
                uint32_t new_v = v[i] | iter;
                changed = changed || new_v != v[i];
                v[i++] = new_v;
            }
            return changed;
        }

        inline bool operator==(const BitVector& other) const
        {
            return v == other.v;
        }

        inline size_t hash() const
        {
            return std::accumulate(v.begin(), v.end(), (uint32_t)0);
        }
    };
}

#endif //NEOAST_CC_COMMON_H
