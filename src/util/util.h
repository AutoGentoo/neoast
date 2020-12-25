//
// Created by tumbar on 12/25/20.
//

#ifndef NEOAST_UTIL_H
#define NEOAST_UTIL_H

#include <lexer.h>
#include <parser.h>
#include <parsergen/canonical_collection.h>

void dump_item(const LR_1* lr1, U32 tok_n, const char* tok_names);
void dump_state(const GrammarState* state, U32 tok_n, U32 lookahead_n, const char* tok_names, U8 line_wrap);
void dump_table(const U32* table, const CanonicalCollection* cc, const char* tok_names, U8 state_wrap);

#endif //NEOAST_UTIL_H
