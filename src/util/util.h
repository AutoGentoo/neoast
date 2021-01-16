//
// Created by tumbar on 12/25/20.
//

#ifndef NEOAST_UTIL_H
#define NEOAST_UTIL_H

#include <lexer.h>
#include <parser.h>
#include <parsergen/canonical_collection.h>

void dump_item(const LR_1* lr1, uint32_t tok_n, const char* tok_names, FILE* fp);
void dump_state(const GrammarState* state, uint32_t tok_n, uint32_t lookahead_n, const char* tok_names, uint8_t line_wrap, FILE* fp);
void dump_table(const uint32_t* table, const CanonicalCollection* cc, const char* tok_names, uint8_t state_wrap, FILE* fp,
                const char* indent_str);

#endif //NEOAST_UTIL_H
