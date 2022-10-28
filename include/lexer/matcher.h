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

#ifndef NEOAST_MATCHER_H
#define NEOAST_MATCHER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new lexer matching engine
 * @param input input to scan over
 * @return Newly allocated token matcher
 */
NeoastMatcher* matcher_new(NeoastInput* input);

/**
 * Destroy a lexer matching engine
 * @param self matcher to destroy
 */
void matcher_free(NeoastMatcher* self);

/**
 * Scan an input using a pattern
 * stream with the set pattern
 * @param self matcher to match with
 * @param pattern_fsm pattern fsm function pointer to match with
 * @return Return group number that was matched in the pattern (0 for none)
 */
size_t matcher_scan(NeoastMatcher* self, NeoastPatternFSM pattern_fsm);

size_t matcher_lineno(NeoastMatcher* self);
size_t matcher_columno(NeoastMatcher* self);
size_t matcher_size(NeoastMatcher* self);
const char* matcher_text(NeoastMatcher* self);
int matcher_input_context(NeoastMatcher* self);

#ifdef __cplusplus
};
#endif

#endif //NEOAST_MATCHER_H