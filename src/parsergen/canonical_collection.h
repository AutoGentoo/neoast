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
    const GrammarParser* parser;
    GrammarState* dfa;

    uint32_t state_s;
    uint32_t state_n;
    GrammarState** all_states;
};

uint8_t lr_1_firstof(uint8_t dest[], uint32_t token, const GrammarParser* parser);
void lookahead_merge(uint8_t dest[], const uint8_t src[], uint32_t n);
CanonicalCollection* canonical_collection_init(const GrammarParser* parser);
void canonical_collection_resolve(CanonicalCollection* self, parser_t p_type);
uint32_t* canonical_collection_generate(const CanonicalCollection* self, const uint8_t* precedence_table);

void canonical_collection_free(CanonicalCollection* self);

#endif //NEOAST_CANONICAL_COLLECTION_H
