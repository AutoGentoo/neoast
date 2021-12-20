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

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-label"
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#include <stdlib.h>
#include <string.h>
#include <neoast.h>
#include "lexer/matcher.h"
#include "lexer/matcher_priv.h"

static inline void matcher_reset_text(NeoastMatcher* self);
static inline size_t matcher_get_1(NeoastMatcher* self, char* s, size_t n);
static inline void matcher_set_current(NeoastMatcher* self, size_t loc);

static inline void matcher_context_init(NeoastMatcherContext* self)
{
    self->buf = NULL;
    self->len = 0;
    self->num = 0;
}

static inline void fsm_init(NeoastMatcherFSM* self)
{
    self->bol = FALSE;
    self->nul = FALSE;
    self->c1 = 0;
}

static inline void option_init(struct Option* self)
{
    self->A = FALSE;
    self->N = FALSE;
    self->W = FALSE;
    self->T = 8;
}

void matcher_init(NeoastMatcher* self)
{
    matcher_context_init(&self->context_);
    fsm_init(&self->fsm_);
    matcher_reset(self);
    option_init(&self->opt_);

    neoast_vector_init(&self->lap_, sizeof(int));
    neoast_vector_init(&self->tab_, sizeof(size_t));
    self->lexing_state = parser_allocate_stack(32);
}

void matcher_destroy(NeoastMatcher* self)
{
    if (self->buf_)
    {
        free(self->buf_);
        self->buf_ = NULL;
    }
    neoast_vector_free(&self->lap_);
    neoast_vector_free(&self->tab_);
    parser_free_stack(self->lexing_state);
}

NeoastMatcher* matcher_new(NeoastInput* input)
{
    NeoastMatcher* self = malloc(sizeof(NeoastMatcher));
    self->in = input;

    matcher_init(self);
    return self;
}

void matcher_free(NeoastMatcher* self)
{
    matcher_destroy(self);
    free(self);
}

void matcher_reset(NeoastMatcher* self)
{
    self->buf_ = NULL;
    self->max_ = 2 * CONST_BLOCK;
    if (posix_memalign((void**) &self->buf_, 4096, self->max_) != 0)
    {
        perror("memalign() - matcher buffer");
        abort();
    }

    self->buf_[0] = '\0';
    self->txt_ = self->buf_;
    self->len_ = 0;
    self->cap_ = 0;
    self->cur_ = 0;
    self->pos_ = 0;
    self->end_ = 0;
    self->ind_ = 0;
    self->blk_ = 0;
    self->got_ = CONST_BOB;
    self->chr_ = '\0';
#if defined(WITH_SPAN)
    self->bol_ = self->buf_;
    self->evh_ = NULL;
#endif
    self->lpb_ = self->buf_;
    self->lno_ = 1;
#if !defined(WITH_SPAN)
    self->cno_ = 0;
#endif
    self->num_ = 0;
    self->eof_ = FALSE;

    self->ded_ = 0;
    self->tab_.n = 0;
}

size_t matcher_scan(NeoastMatcher* self, NeoastPatternFSM fsm)
/// @returns nonzero if input matched the pattern
{
    matcher_reset_text(self);
    self->len_ = 0;     // split text length starts with 0
    scan:
    self->txt_ = self->buf_ + self->cur_;
#if !defined(WITH_NO_INDENT)
    self->mrk_ = FALSE;
    self->ind_ = self->pos_; // ind scans input in buf[] in newline() up to pos - 1
    self->col_ = 0; // count columns for indent matching
#endif
    find:;
    int c1 = self->got_;
    bool_t bol = matcher_at_bol(self); // at begin of line?

    assert(fsm);
    self->fsm_.c1 = c1;
#if !defined(WITH_NO_INDENT)
    redo:
#endif
    neoast_vector_resize(&self->lap_, 0, 0);
    self->cap_ = 0;
    bool_t nul = FALSE;

    // Run the fsm matching
    self->fsm_.bol = bol;
    self->fsm_.nul = nul;
    fsm(self);
    nul = self->fsm_.nul;
    c1 = self->fsm_.c1;

#if !defined(WITH_NO_INDENT)
    if (self->mrk_ && self->cap_ != CONST_REDO)
    {
        if (self->col_ > 0 && (self->tab_.n == 0 || VECTOR_BACK(self->tab_, size_t) < self->col_))
        {
            DBGLOG("Set new stop: tab_[%zu] = %zu", self->tab_.n, self->col_);
            neoast_vector_push_back_s(&self->tab_, self->col_);
        }
        else if (self->tab_.n && VECTOR_BACK(self->tab_, size_t) > self->col_)
        {
            size_t n;
            for (n = self->tab_.n - 1; n > 0; --n)
                if (VECTOR_AT(self->tab_, n - 1, size_t) <= self->col_)
                    break;
            self->ded_ += self->tab_.n - n;
            DBGLOG("Dedents: ded = %zu tab_ = %zu", self->ded_, self->tab_.n);
            neoast_vector_resize(&self->tab_, n, -1);
// adjust stop when indents are not aligned (Python would give an error though)
            if (n > 0)
                VECTOR_BACK(self->tab_, size_t) = self->col_;
        }
    }
    if (self->ded_ > 0)
    {
        DBGLOG("Dedents: ded = %zu", self->ded_);
        if (self->col_ == 0 && bol)
        {
            self->ded_ += self->tab_.n;
            neoast_vector_resize(&self->tab_, 0, 0);
            DBGLOG("Rescan for pending dedents: ded = %zu", self->ded_);
            self->pos_ = self->ind_;
// avoid looping, match \j exactly
            bol = FALSE;
            goto redo;
        }
        --self->ded_;
    }
#endif
    if (self->cap_ == 0)
    {
        // no match: backup to begin of unmatched text
        self->cur_ = self->txt_ - self->buf_;
    }
    self->len_ = self->cur_ - (self->txt_ - self->buf_);
    if (self->len_ == 0 && !nul)
    {
        DBGLOG("Empty or no match cur = %zu pos = %zu end = %zu", self->cur_, self->pos_, self->end_);
        self->pos_ = self->cur_;
        if (matcher_at_end(self))
        {
            matcher_set_current(self, self->cur_);
            DBGLOG("Reject empty match at EOF");
            self->cap_ = 0;
        }
        else
        {
            matcher_set_current(self, self->cur_);
            DBGLOG("Reject empty match");
            self->cap_ = 0;
        }
    }
    else if (self->len_ == 0 && self->cur_ == self->end_)
    {
        DBGLOG("Hit end: got = %d", self->got_);
        if (self->cap_ == CONST_REDO && !self->opt_.A)
            self->cap_ = 0;
    }
    else
    {
        matcher_set_current(self, self->cur_);
        if (self->len_ > 0 && self->cap_ == CONST_REDO && !self->opt_.A)
        {
            DBGLOG("Ignore accept and continue: len = %zu", self->len_);
            self->len_ = 0;
            goto scan;
        }
    }

    //    DBGLOG("Return: cap = %zu txt = '%s' len = %zu pos = %zu got = %d", self->cap_, std::string(txt_, len_).c_str(), len_,
    //           pos_, got_);
    DBGLOG("END match()");
    return self->cap_;
}

/// Shift or expand the internal buffer when it is too small to accommodate more input, where the buffer size is doubled when needed, change cur_, pos_, end_, max_, ind_, buf_, bol_, lpb_, and txt_.
static inline bool_t matcher_grow(NeoastMatcher* self, size_t need) ///< optional needed space = Const::BLOCK size by default
/// @returns true if buffer was shifted or enlarged
{
    if (self->max_ - self->end_ >= need + 1)
        return FALSE;
#if defined(WITH_SPAN)
    (void)lineno();
    if (self->bol_ + CONST_BLOCK < self->txt_ && self->evh_ == NULL)
    {
      // this line is very long, likely a binary file, so shift a block size away from the match instead
      DBGLOG("Line in buffer to long to shift, moving bol position to text match position minus %zu", Const::BLOCK);
        self->bol_ = self->txt_ - CONST_BLOCK;
    }
    size_t gap = self->bol_ - self->buf_;
    if (gap > 0 && self->evh_ != NULL)
      (*self->evh_)(self, self->buf_, gap, self->num_);
    self->cur_ -= gap;
    self->ind_ -= gap;
    self->pos_ -= gap;
    self->end_ -= gap;
    self->txt_ -= gap;
    self->bol_ -= gap;
    self->lpb_ -= gap;
    self->num_ += gap;
    memmove(self->buf_, self->buf_ + gap, self->end_);
    if (self->max_ - self->end_ >= need)
    {
      DBGLOG("Shift buffer to close gap of %zu bytes", gap);
    }
    else
    {
      size_t newmax = self->end_ + need;
      while (self->max_ < newmax)
          self->max_ *= 2;
      DBGLOG("Expand buffer to %zu bytes", max_);
#if defined(WITH_REALLOC)
#if (defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__BORLANDC__)) && !defined(__CYGWIN__)
      char *newbuf = static_cast<char*>(_aligned_realloc(static_cast<void*>(buf_), max_, 4096));
#else
      char *newbuf = static_cast<char*>(std::realloc(static_cast<void*>(buf_), max_));
#endif
      if (newbuf == NULL)
        throw std::bad_alloc();
#else
      char *newbuf = new char[max_];
      std::memcpy(newbuf, buf_, end_);
      delete[] buf_;
#endif
      txt_ = newbuf + (txt_ - buf_);
      lpb_ = newbuf + (lpb_ - buf_);
      buf_ = newbuf;
    }
    bol_ = buf_;
#else
    size_t gap = self->txt_ - self->buf_;
    if (self->max_ - self->end_ + gap >= need)
    {
        DBGLOG("Shift buffer to close gap of %zu bytes", gap);
        (void) matcher_lineno(self);
        self->cur_ -= gap;
        self->ind_ -= gap;
        self->pos_ -= gap;
        self->end_ -= gap;
        self->num_ += gap;
        if (self->end_ > 0)
            memmove(self->buf_, self->txt_, self->end_);
        self->txt_ = self->buf_;
        self->lpb_ = self->buf_;
    }
    else
    {
        size_t newmax = self->end_ - gap + need;
        size_t oldmax = self->max_;
        while (self->max_ < newmax)
            self->max_ <<= 1;
        if (oldmax < self->max_)
        {
            DBGLOG("Expand buffer from %zu to %zu bytes", oldmax, max_);
            (void) matcher_lineno(self);
            self->cur_ -= gap;
            self->ind_ -= gap;
            self->pos_ -= gap;
            self->end_ -= gap;
            self->num_ += gap;
            memmove(self->buf_, self->txt_, self->end_);
            char *newbuf = (char*)(realloc((void*)self->buf_, self->max_));
            if (newbuf == NULL)
                assert(0 && "bad allocation");
            self->buf_ = newbuf;
            self->txt_ = self->buf_;
            self->lpb_ = self->buf_;
        }
    }
#endif
    return TRUE;
}

/// Get the next character and grow the buffer to make more room if necessary.
int matcher_get_more(NeoastMatcher* self)
/// @returns the character read (unsigned char 0..255) or EOF (-1)
{
    if (self->eof_)
        return EOF;
    while (TRUE)
    {
        if (self->end_ + self->blk_ + 1 >= self->max_)
            (void) matcher_grow(self, CONST_BLOCK);
        self->end_ += matcher_get_1(self, self->buf_ + self->end_,
                                    self->blk_ > 0 ? self->blk_ : self->max_ - self->end_ - 1);
        if (self->pos_ < self->end_)
            return (unsigned char) (self->buf_[self->pos_++]);
        self->eof_ = TRUE;
        return EOF;
    }
}

/// Reset the matched text by removing the terminating \0, which is needed to search for a new match.
static inline void matcher_reset_text(NeoastMatcher* self)
{
    if (self->chr_ != '\0')
    {
        self->txt_[self->len_] = self->chr_;
        self->chr_ = '\0';
    }
}


/// Updates and returns the starting line number of the match in the input character sequence.
size_t matcher_lineno(NeoastMatcher* self)
/// @returns line number
{
#if defined(WITH_SPAN)
    if (self->lpb_ < self->txt_)
    {
        char* s = self->bol_;
        char* t = self->txt_;
        // clang/gcc 4-way vectorizable loop
        while (t - 4 >= s)
        {
            if ((t[-1] == '\n') | (t[-2] == '\n') | (t[-3] == '\n') | (t[-4] == '\n'))
                break;
            t -= 4;
        }
        // epilogue
        if (--t >= s && *t != '\n')
            if (--t >= s && *t != '\n')
                if (--t >= s && *t != '\n')
                    --t;
        self->bol_ = t + 1;
        self->lpb_ = self->txt_;
        size_t n = self->lno_;
#if defined(HAVE_AVX512BW) && (!defined(_MSC_VER) || defined(_WIN64))
        if (have_HW_AVX512BW())
        {
          __m512i vlcn = _mm512_set1_epi8('\n');
          while (s + 63 <= t)
          {
            __m512i vlcm = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(s));
            uint64_t mask = _mm512_cmpeq_epi8_mask(vlcm, vlcn);
            n += popcountl(mask);
            s += 64;
          }
        }
        else if (have_HW_AVX2())
        {
          __m256i vlcn = _mm256_set1_epi8('\n');
          while (s + 31 <= t)
          {
            __m256i vlcm = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s));
            __m256i vlceq = _mm256_cmpeq_epi8(vlcm, vlcn);
            uint32_t mask = _mm256_movemask_epi8(vlceq);
            n += popcount(mask);
            s += 32;
          }
        }
        else if (have_HW_SSE2())
        {
          __m128i vlcn = _mm_set1_epi8('\n');
          while (s + 15 <= t)
          {
            __m128i vlcm = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));
            __m128i vlceq = _mm_cmpeq_epi8(vlcm, vlcn);
            uint32_t mask = _mm_movemask_epi8(vlceq);
            n += popcount(mask);
            s += 16;
          }
        }
#elif defined(HAVE_AVX2)
        if (have_HW_AVX2())
        {
          __m256i vlcn = _mm256_set1_epi8('\n');
          while (s + 31 <= t)
          {
            __m256i vlcm = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s));
            __m256i vlceq = _mm256_cmpeq_epi8(vlcm, vlcn);
            uint32_t mask = _mm256_movemask_epi8(vlceq);
            n += popcount(mask);
            s += 32;
          }
        }
        else if (have_HW_SSE2())
        {
          __m128i vlcn = _mm_set1_epi8('\n');
          while (s + 15 <= t)
          {
            __m128i vlcm = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));
            __m128i vlceq = _mm_cmpeq_epi8(vlcm, vlcn);
            uint32_t mask = _mm_movemask_epi8(vlceq);
            n += popcount(mask);
            s += 16;
          }
        }
#elif defined(HAVE_SSE2)
        if (have_HW_SSE2())
        {
          __m128i vlcn = _mm_set1_epi8('\n');
          while (s + 15 <= t)
          {
            __m128i vlcm = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));
            __m128i vlceq = _mm_cmpeq_epi8(vlcm, vlcn);
            uint32_t mask = _mm_movemask_epi8(vlceq);
            n += popcount(mask);
            s += 16;
          }
        }
#elif defined(HAVE_NEON)
        {
          // ARM AArch64/NEON SIMD optimized loop? - no code found yet that runs faster than the code below
        }
#endif
        uint32_t n0 = 0, n1 = 0, n2 = 0, n3 = 0;
        // clang/gcc 4-way vectorizable loop
        while (s + 3 <= t)
        {
            n0 += s[0] == '\n';
            n1 += s[1] == '\n';
            n2 += s[2] == '\n';
            n3 += s[3] == '\n';
            s += 4;
        }
        n += n0 + n1 + n2 + n3;
        // epilogue
        if (s <= t)
        {
            n += *s == '\n';
            if (++s <= t)
            {
                n += *s == '\n';
                if (++s <= t)
                    n += *s == '\n';
            }
        }
        self->lno_ = n;
    }
#else
    size_t n = self->lno_;
    size_t k = self->cno_;
    char* s = self->lpb_;
    char* e = self->txt_;
    while (s < e)
    {
        if (*s == '\n')
        {
            ++n;
            k = 0;
        }
        else if (*s == '\t')
        {
            // count tab spacing
            k += 1 + (~k & (self->opt_.T - 1));
        }
        else
        {
            // count column offset in UTF-8 chars
            k += ((*s & 0xC0) != 0x80);
        }
        ++s;
    }
    self->lpb_ = e;
    self->lno_ = n;
    self->cno_ = k;
#endif
    return self->lno_;
}

size_t matcher_columno(NeoastMatcher* self)
{
    (void)matcher_lineno(self);
#if defined(WITH_SPAN)
    const char *s = self->cpb_;
    const char *e = self->txt_;
    size_t k = self->cno_;
    size_t m = self->opt_.T - 1;
    while (s < e)
    {
      if (*s == '\t')
        k += 1 + (~k & m); // count tab spacing
      else
        k += ((*s & 0xC0) != 0x80); // count column offset in UTF-8 chars
      ++s;
    }
    self->cpb_ = self->txt_;
    self->cno_ = k;
#endif
    return self->cno_;
}

size_t matcher_size(NeoastMatcher* self)
{
    return self->len_;
}

const char* matcher_text(NeoastMatcher* self)
{
    if (self->chr_ == '\0')
    {
        self->chr_ = self->txt_[self->len_];
        self->txt_[self->len_] = '\0';
    }
    return self->txt_;
}

static inline size_t matcher_get_1(NeoastMatcher* self, char* s, size_t n)
{
    return input_get(self->in, s, n);
}

/// Set the current position in the buffer for the next match.
static inline void matcher_set_current(NeoastMatcher* self, size_t loc) ///< new location in buffer
{
//    DBGCHK(loc <= end_);
    self->pos_ = self->cur_ = loc;
#if defined(WITH_SPAN)
    self->got_ = loc > 0 ? (unsigned char)(self->buf_[loc - 1]) : '\n';
#else
    self->got_ = loc > 0 ? (unsigned char) (self->buf_[loc - 1]) : CONST_UNK;
#endif
}

/// Set the current match position in the buffer.
static inline void matcher_set_current_match(NeoastMatcher* self, size_t loc) ///< new location in buffer
{
    matcher_set_current(self, loc);
    self->txt_ = self->buf_ + self->cur_;
}

/// Peek at the next character and grow the buffer to make more room if necessary.
int matcher_peek_more(NeoastMatcher* self)
/// @returns the character (unsigned char 0..255) or EOF (-1)
{
    DBGLOG("AbstractMatcher::peek_more()");
    if (self->eof_)
        return EOF;
    while (TRUE)
    {
        if (self->end_ + self->blk_ + 1 >= self->max_)
            (void) matcher_grow(self, CONST_BLOCK);
        self->end_ += matcher_get_1(self, self->buf_ + self->end_,
                                    self->blk_ > 0 ? self->blk_ : self->max_ - self->end_ - 1);
        if (self->pos_ < self->end_)
            return (unsigned char) (self->buf_[self->pos_]);
//        DBGLOGN("peek_more(): EOF");
        self->eof_ = TRUE;
        return EOF;
    }
}
