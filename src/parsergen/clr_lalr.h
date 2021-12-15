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


#ifndef NEOAST_CLR_LALR_H
#define NEOAST_CLR_LALR_H

#include "canonical_collection.h"

int lalr_1_cmp(const GrammarState* a, const GrammarState* b, uint32_t tok_n);
void lalr_1_merge(GrammarState* target, const GrammarState* to_merge, uint32_t tok_n);
int clr_1_cmp(const GrammarState* a, const GrammarState* b, uint32_t tok_n);

#endif //NEOAST_CLR_LALR_H
