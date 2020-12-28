//
// Created by tumbar on 12/24/20.
//

#ifndef NEOAST_CANONICAL_COLLECTION_H
#define NEOAST_CANONICAL_COLLECTION_H

#include "common.h"
#include "parser.h"

typedef enum {
    LALR_1,  // Highly recommended!!
    CLR_1,
} parser_t;

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

U8 lr_1_firstof(U8 dest[], U32 token, const GrammarParser* parser);
void lookahead_merge(U8 dest[], const U8 src[], U32 n);
CanonicalCollection* canonical_collection_init(const GrammarParser* parser);
void canonical_collection_resolve(CanonicalCollection* self, parser_t p_type);
U32* canonical_collection_generate(const CanonicalCollection* self, const U8* precedence_table);

void canonical_collection_free(CanonicalCollection* self);

#endif //NEOAST_CANONICAL_COLLECTION_H
