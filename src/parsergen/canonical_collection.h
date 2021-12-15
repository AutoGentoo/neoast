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

#ifdef __cplusplus
extern "C" {
#endif

#include "neoast.h"

// Codegen
typedef struct LR_1_prv LR_1;
typedef struct GrammarState_prv GrammarState;
typedef struct CanonicalCollection_prv CanonicalCollection;
typedef struct DebugInfo_prv DebugInfo;

typedef enum {
    LALR_1,  // Highly recommended!!
    CLR_1,
} parser_t;

struct LR_1_prv
{
    const GrammarRule* grammar;
    uint32_t rule_index;
    LR_1* next;
    uint32_t item_i;
    uint8_t final_item;
    uint8_t look_ahead[]; // List of booleans for each
                          // token to treat as a lookahead
};

struct GrammarState_prv
{
    LR_1* head_item;
    GrammarState* action_states[];
};

struct CanonicalCollection_prv
{
    DebugInfo* debug_info;
    const GrammarParser* parser;
    GrammarState* dfa;

    uint32_t state_s;
    uint32_t state_n;
    GrammarState** all_states;
};

struct DebugInfo_prv
{
    const TokenPosition** grammar_rule_positions;
};

void lookahead_merge(uint8_t dest[], const uint8_t src[], uint32_t n);

DebugInfo* debug_info_init(const TokenPosition** grammar_rule_positions);
void debug_info_free(DebugInfo* self);

CanonicalCollection* canonical_collection_init(const GrammarParser* parser, DebugInfo* debug_info);
void canonical_collection_resolve(CanonicalCollection* self, parser_t p_type);
uint32_t* canonical_collection_generate(const CanonicalCollection* self, const uint8_t* precedence_table, uint8_t* error);

void canonical_collection_free(CanonicalCollection* self);

#ifdef __cplusplus
};
#endif

#endif //NEOAST_CANONICAL_COLLECTION_H
