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

#ifndef NEOAST_MATCHER_PRIV_H
#define NEOAST_MATCHER_PRIV_H

#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "common.h"
#include "container.h"
#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NeoastMatcherContext_prv
{
    const char* buf;    ///< pointer to buffer
    size_t len;         ///< length of buffered context
    size_t num;         ///< number of bytes shifted out so far, when buffer shifted
};

struct NeoastMatcherFSM_prv
{
    bool_t bol;
    bool_t nul;
    int c1;
};

struct NeoastMatcher_prv
{
    NeoastMatcherContext context_;
    NeoastInput* in;

    struct Option
    {
        bool_t A; ///< accept any/all (?^X) negative patterns as Const::REDO accept index codes
        bool_t N; ///< nullable, find may return empty match (N/A to scan, split, matches)
        bool_t W; ///< half-check for "whole words", check only left of \< and right of \> for non-word character
        char T; ///< tab size, must be a power of 2, default is 8, for column count and indent \i, \j, and \k
    } opt_;

    char* buf_;      ///< input character sequence buffer
    char* txt_;      ///< points to the matched text in buffer AbstractMatcher::buf_
    size_t len_;     ///< size of the matched text
    size_t cap_;     ///< nonzero capture index of an accepted match or zero
    size_t max_;     ///< total buffer size and max position + 1 to fill
    size_t cur_;     ///< next position in AbstractMatcher::buf_ to assign to AbstractMatcher::txt_
    size_t pos_;     ///< position in AbstractMatcher::buf_ after AbstractMatcher::txt_
    size_t end_;     ///< ending position of the input buffered in AbstractMatcher::buf_
    size_t ind_;     ///< current indent position
    size_t blk_;     ///< block size for block-based input reading, as set by AbstractMatcher::buffer
    int got_;        ///< last unsigned character we looked at (to determine anchors and boundaries)
    int chr_;        ///< the character located at AbstractMatcher::txt_[AbstractMatcher::len_]

//#define WITH_SPAN
#if defined(WITH_SPAN)
    char     *bol_;  ///< begin of line pointer in buffer
    Handler  *evh_;    ///< event handler functor to invoke when buffer contents are shifted out
#endif
    char* lpb_;      ///< line pointer in buffer, updated when counting line numbers with lineno()
    size_t lno_;     ///< line number count (cached)
#if !defined(WITH_SPAN)
    size_t cno_;     ///< column number count (cached)
#endif
    size_t num_;     ///< character count of the input till bol_
    bool_t own_;     ///< true if AbstractMatcher::buf_ was allocated and should be deleted
    bool_t eof_;     ///< input has reached EOF
    bool_t mat_;     ///< true if AbstractMatcher::matches() was successful

    size_t ded_;      ///< dedent count
    size_t col_;      ///< column counter for indent matching, updated by newline(), indent(), and dedent()
    NeoastVector tab_;      ///< tab stops set by detecting indent margins
    NeoastVector lap_;      ///< lookahead position in input that heads a lookahead match (indexed by lookahead number)
//    NeoastStack             stk_;      ///< stack to push/pop stops
    NeoastMatcherFSM fsm_;  ///< local state for FSM code
    uint16_t lcp_;    ///< primary least common character position in the pattern prefix or 0xffff for pure Boyer-Moore
    uint16_t lcs_;    ///< secondary least common character position in the pattern prefix or 0xffff for pure Boyer-Moore
    size_t bmd_;      ///< Boyer-Moore jump distance on mismatch, B-M is enabled when bmd_ > 0
    uint8_t bms_[256]; ///< Boyer-Moore skip array
    bool_t mrk_;      ///< indent \i or dedent \j in pattern found: should check and update indent stops
    bool_t anc_;      ///< match is anchored, advance slowly to retry when searching
};

void matcher_reset(NeoastMatcher* self);

int matcher_get_more(NeoastMatcher* self);

static inline int matcher_get(NeoastMatcher* self);

/// Check ASCII word-like character `[A-Za-z0-9_]`, permitting the character range 0..303 (0x12F) and EOF.
static inline int isword(int c) ///< Character to check
/// @returns nonzero if argument c is in `[A-Za-z0-9_]`, zero otherwise
{
    return isalnum((unsigned char) (c)) | (c == '_');
}

int matcher_peek_more(NeoastMatcher* self);

#if !defined(WITH_NO_INDENT)

/// Update indentation column counter for indent() and dedent().
static inline void matcher_newline(NeoastMatcher* self)
{
    self->mrk_ = TRUE;
    while (self->ind_ + 1 < self->pos_)
        self->col_ += self->buf_[self->ind_++] == '\t' ? 1 + (~self->col_ & (self->opt_.T - 1)) : 1;
}

/// Returns true if looking at indent.
static inline bool_t matcher_indent(NeoastMatcher* self)
/// @returns true if indent
{
    matcher_newline(self);
    return self->col_ > 0 && (
            self->tab_.n == 0 || // empty
            VECTOR_AT(self->tab_, self->tab_.n - 1, size_t) < self->col_
    );
}

/// Returns true if looking at dedent.
static inline bool_t matcher_dedent(NeoastMatcher* self)
/// @returns true if dedent
{
    matcher_newline(self);
    return self->tab_.n && VECTOR_AT(self->tab_, self->tab_.n - 1, size_t) > self->col_;
}

/// Returns true if nodent.
static inline bool_t matcher_nodent(NeoastMatcher* self)
/// @returns true if nodent
{
    matcher_newline(self);
    return (self->col_ <= 0 || (self->tab_.n && VECTOR_AT(self->tab_, self->tab_.n - 1, size_t) >= self->col_)) &&
           (self->tab_.n == 0 || VECTOR_AT(self->tab_, self->tab_.n - 1, size_t) <= self->col_);
}

#endif

static inline int matcher_get(NeoastMatcher* self)
{
    return self->pos_ < self->end_ ? (self->buf_[self->pos_++]) : matcher_get_more(self);
}

/// Peek at the next character available for reading from the current input source.
static inline int matcher_peek(NeoastMatcher* self)
/// @returns the character (unsigned char 0..255) or EOF (-1)
{
    return self->pos_ < self->end_ ? (unsigned char) (self->buf_[self->pos_]) : matcher_peek_more(self);
}

/// Returns true if this matcher has no more input to read from the input character sequence.
static inline bool_t matcher_at_end(NeoastMatcher* self)
/// @returns true if at end of input and a read attempt will produce EOF
{
    return self->pos_ >= self->end_ && (self->eof_ || matcher_peek(self) == EOF);
}

/// Returns true if this matcher is at the start of a buffer to read an input character sequence. Use reset() to restart reading new input.
static inline bool_t matcher_at_bob(const NeoastMatcher* self)
/// @returns true if at the begin of an input sequence
{
    return self->got_ == CONST_BOB;
}

/// Returns true if this matcher reached the begin of a new line.
static inline bool_t matcher_at_bol(const NeoastMatcher* self)
/// @returns true if at begin of a new line
{
    return self->got_ == CONST_BOB || self->got_ == '\n';
}

/// Returns true if this matcher hit the end of the input character sequence.
static inline bool_t matcher_hit_end(const NeoastMatcher* self)
/// @returns true if EOF was hit (and possibly more input would have changed the result), false otherwise (but next read attempt may return EOF immediately)
{
    return self->pos_ >= self->end_ && self->eof_;
}

#ifdef __cplusplus
};
#endif

#endif //NEOAST_MATCHER_PRIV_H
