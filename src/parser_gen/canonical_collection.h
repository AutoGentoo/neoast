//
// Created by tumbar on 12/24/20.
//

#ifndef NEOAST_CANONICAL_COLLECTION_H
#define NEOAST_CANONICAL_COLLECTION_H

#include "common.h"

typedef int (*state_compare)(
        const GrammarState* original_state,
        const GrammarState* potential_duplicate,
        U32 tok_n);

typedef void (*state_merge)(
        GrammarState* target,
        const GrammarState* to_merge,
        U32 tok_n);

struct LR_1_prv
{
    const GrammarRule* grammar;
    LR_1* next;
    U32 item_i;
    U8 final_item;
    U8 look_ahead[]; // List of booleans for each
                     // token to treat as a lookahead
};

struct GrammarState_prv
{
    LR_1* head_item;
    GrammarState* action_states[];
};

struct CanonicalCollection_prv
{
    const GrammarParser* parser;
    GrammarState* dfa;

    U32 state_s;
    U32 state_n;
    GrammarState* all_states[];
};

CanonicalCollection* canonical_collection_init(const GrammarParser* parser);
void canonical_collection_resolve(CanonicalCollection* self, state_compare cmp_f, state_merge cmp_m);
U32* canonical_collection_generate(const CanonicalCollection* self);

void canonical_collection_free(CanonicalCollection* self);

#endif //NEOAST_CANONICAL_COLLECTION_H
