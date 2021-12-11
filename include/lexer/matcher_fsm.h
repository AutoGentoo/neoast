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

#ifndef NEOAST_MATCHER_FSM_H
#define NEOAST_MATCHER_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "matcher_priv.h"

/// FSM code INIT.
static inline void FSM_INIT(NeoastMatcher* m, int* c1)
{
    *c1 = m->fsm_.c1;
}

/// FSM code FIND.
static inline void FSM_FIND(NeoastMatcher* m)
{
    if (m->cap_ == 0)
        m->cur_ = m->pos_;
}

/// FSM code CHAR.
static inline int FSM_CHAR(NeoastMatcher* m)
{
    return matcher_get(m);
}

/// FSM code HALT.
static inline void FSM_HALT(NeoastMatcher* m, int c1)
{
    m->fsm_.c1 = c1;
}

/// FSM code TAKE.
static inline void FSM_TAKE(NeoastMatcher* m, NeosastPatternAccept cap, int c1)
{
    m->cap_ = cap;
    m->cur_ = m->pos_;
    if (c1 != EOF)
        --m->cur_;
}

/// FSM code REDO.
static inline void FSM_REDO(NeoastMatcher* m, int c1)
{
    m->cap_ = CONST_REDO;
    m->cur_ = m->pos_;
    if (c1 != EOF)
        --m->cur_;
}

/// FSM code HEAD.
static inline void FSM_HEAD(NeoastMatcher* m, NeoastPatternLookahead la)
{
    if (m->lap_.n <= la)
        vector_resize(&m->lap_, la + 1, -1);
    VECTOR_AT(m->lap_, la, int) = (int) (m->pos_ - (m->txt_ - m->buf_));
}

/// FSM code TAIL.
static inline void FSM_TAIL(NeoastMatcher* m, NeoastPatternLookahead la)
{
    if (m->lap_.n > la && VECTOR_AT(m->lap_, la, size_t) >= 0)
        m->cur_ = m->txt_ - m->buf_ + VECTOR_AT(m->lap_, la, size_t);
}

/// FSM code DENT.
static inline bool_t FSM_DENT(NeoastMatcher* m)
{
    if (m->ded_ > 0)
    {
        m->fsm_.nul = TRUE;
        return TRUE;
    }
    return FALSE;
}

/// FSM extra code POSN returns current position.
static inline size_t FSM_POSN(NeoastMatcher* m)
{
    return m->pos_ - (m->txt_ - m->buf_);
}

/// FSM extra code BACK position to a previous position returned by FSM_POSN().
static inline void FSM_BACK(NeoastMatcher* m, size_t pos)
{
    m->cur_ = m->txt_ - m->buf_ + pos;
}

#if !defined(WITH_NO_INDENT)

/// FSM code META DED.
static inline bool_t FSM_META_DED(NeoastMatcher* m)
{
    return m->fsm_.bol && matcher_dedent(m);
}

/// FSM code META IND.
static inline bool_t FSM_META_IND(NeoastMatcher* m)
{
    return m->fsm_.bol && matcher_indent(m);
}

/// FSM code META UND.
static inline bool_t FSM_META_UND(NeoastMatcher* m)
{
    bool_t mrk = m->mrk_ && !matcher_nodent(m);
    m->mrk_ = FALSE;
    m->ded_ = 0;
    return mrk;
}

#endif

/// FSM code META EOB.
static inline bool_t FSM_META_EOB(NeoastMatcher* m, int c1)
{
    return c1 == EOF;
}

/// FSM code META BOB.
static inline bool_t FSM_META_BOB(NeoastMatcher* m)
{
    return matcher_at_bob(m);
}

/// FSM code META EOL.
static inline bool_t FSM_META_EOL(NeoastMatcher* m, int c1)
{
    m->anc_ = TRUE;
    return c1 == EOF || c1 == '\n' || (c1 == '\r' && matcher_peek(m) == '\n');
}

/// FSM code META BOL.
static inline bool_t FSM_META_BOL(NeoastMatcher* m)
{
    m->anc_ = TRUE;
    return m->fsm_.bol;
}

/// FSM code META EWE.
static inline bool_t FSM_META_EWE(NeoastMatcher* m, int c0, int c1)
{
    m->anc_ = TRUE;
    return (isword(c0) || m->opt_.W) && !isword(c1);
}

/// FSM code META BWE.
static inline bool_t FSM_META_BWE(NeoastMatcher* m, int c0, int c1)
{
    m->anc_ = TRUE;
    return !isword(c0) && isword(c1);
}

/// FSM code META EWB.
static inline bool_t FSM_META_EWB(NeoastMatcher* m)
{
    m->anc_ = TRUE;
    return isword(m->got_) && !isword((unsigned char) (m->txt_[m->len_]));
}

/// FSM code META BWB.
static inline bool_t FSM_META_BWB(NeoastMatcher* m)
{
    m->anc_ = TRUE;
    return !isword(m->got_) && (m->opt_.W || isword((unsigned char) (m->txt_[m->len_])));
}

/// FSM code META NWE.
static inline bool_t FSM_META_NWE(NeoastMatcher* m, int c0, int c1)
{
    m->anc_ = TRUE;
    return isword(c0) == isword(c1);
}

/// FSM code META NWB.
static inline bool_t FSM_META_NWB(NeoastMatcher* m)
{
    m->anc_ = TRUE;
    return isword(m->got_) == isword((unsigned char) (m->txt_[m->len_]));
}

#ifdef __cplusplus
};
#endif

#endif //NEOAST_MATCHER_FSM_H
