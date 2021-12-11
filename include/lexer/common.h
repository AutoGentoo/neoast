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

#ifndef NEOAST_COMMON_H
#define NEOAST_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// matcher.h
typedef struct NeoastMatcherContext_prv NeoastMatcherContext;
typedef struct NeoastMatcher_prv NeoastMatcher;
typedef struct NeoastMatcherFSM_prv NeoastMatcherFSM;

// container.h
typedef struct NeoastVector_prv NeoastVector;
typedef NeoastVector NeoastStack;

// input.h
typedef struct NeoastInput_prv NeoastInput;

// pattern.h
typedef void (*NeoastPatternFSM)(NeoastMatcher*); ///< function pointer to FSM code

typedef uint16_t NeoastPatternLookahead;
typedef uint32_t NeosastPatternAccept; ///< group capture index

#define DBGLOG(...) (void) 0

///< NUL string terminator
#define CONST_NUL '\0'

///< unknown/undefined character meta-char marker
#define CONST_UNK 256

///< begin of buffer meta-char marker
#define CONST_BOB 257

///< end of buffer meta-char marker
#define CONST_EOB EOF

///< buffer size and growth, buffer is initially 2*BLOCK size, at least 4096 bytes
#define CONST_BLOCK (256 * 1024)

///< reflex::NeoastMatcher::accept() returns "redo" with reflex::NeoastMatcher option "A"
#define CONST_REDO 0x7FFFFFFF

///< accept() returns "empty" last split at end of input
#define CONST_EMPTY 0xFFFFFFFF

typedef int bool_t;
#define TRUE (1)
#define FALSE (0)

#ifdef __cplusplus
};
#endif

#endif //NEOAST_COMMON_H
