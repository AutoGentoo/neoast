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


#ifndef NEOAST_UTIL_H
#define NEOAST_UTIL_H

#include <neoast.h>
#include <parsergen/canonical_collection.h>

void dump_item(const LR_1* lr1, uint32_t tok_n, const char* tok_names, FILE* fp);
void dump_state(const GrammarState* state, uint32_t tok_n, uint32_t lookahead_n, const char* tok_names, uint8_t line_wrap, FILE* fp);
void dump_table(const uint32_t* table, const CanonicalCollection* cc, const char* tok_names, uint8_t state_wrap, FILE* fp,
                const char* indent_str);

#endif //NEOAST_UTIL_H
