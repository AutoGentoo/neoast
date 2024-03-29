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

#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include "lexer/input.h"

#if defined(WITH_STANDARD_REPLACEMENT_CHARACTER)
/// Replace invalid UTF-8 with the standard replacement character U+FFFD.  This is not the default in RE/flex.
# define REFLEX_NONCHAR      (0xFFFD)
# define REFLEX_NONCHAR_UTF8 "\xef\xbf\xbd"
#else
/// Replace invalid UTF-8 with the non-character U+200000 code point for guaranteed error detection (the U+FFFD code point makes error detection harder and possible to miss).
# define REFLEX_NONCHAR      (0x200000)
# define REFLEX_NONCHAR_UTF8 "\xf8\x88\x80\x80\x80"
#endif


/// Convert UCS-4 to UTF-8, fills with REFLEX_NONCHAR_UTF8 when out of range, or unrestricted UTF-8 with WITH_UTF8_UNRESTRICTED.
static inline size_t utf8(
        int c, ///< UCS-4 character U+0000 to U+10ffff (unless WITH_UTF8_UNRESTRICTED)
        char* s) ///< points to the buffer to populate with UTF-8 (1 to 6 bytes) not NUL-terminated
/// @returns length (in bytes) of UTF-8 character sequence stored in s
{
    if (c < 0x80)
    {
        *s++ = (char) (c);
        return 1;
    }
#ifndef WITH_UTF8_UNRESTRICTED
    if (c > 0x10FFFF)
    {
        static const size_t n = sizeof(REFLEX_NONCHAR_UTF8) - 1;
        memcpy(s, REFLEX_NONCHAR_UTF8, n);
        return n;
    }
#endif
    char* t = s;
    if (c < 0x0800)
    {
        *s++ = (char) (0xC0 | ((c >> 6) & 0x1F));
    }
    else
    {
        if (c < 0x010000)
        {
            *s++ = (char) (0xE0 | ((c >> 12) & 0x0F));
        }
        else
        {
            size_t w = c;
#ifndef WITH_UTF8_UNRESTRICTED
            *s++ = (char) (0xF0 | ((w >> 18) & 0x07));
#else
            if (c < 0x200000)
      {
        *s++ = static_cast<char>(0xF0 | ((w >> 18) & 0x07));
      }
      else
      {
        if (w < 0x04000000)
        {
          *s++ = static_cast<char>(0xF8 | ((w >> 24) & 0x03));
        }
        else
        {
          *s++ = static_cast<char>(0xFC | ((w >> 30) & 0x01));
          *s++ = static_cast<char>(0x80 | ((w >> 24) & 0x3F));
        }
        *s++ = static_cast<char>(0x80 | ((w >> 18) & 0x3F));
      }
#endif
            *s++ = (char) (0x80 | ((c >> 12) & 0x3F));
        }
        *s++ = (char) (0x80 | ((c >> 6) & 0x3F));
    }
    *s++ = (char) (0x80 | (c & 0x3F));
    return s - t;
}


size_t file_get(struct FileHandle* self, char* s, size_t n)
{
    char* t = s;
    if (self->ulen_ > 0)
    {
        size_t k = n < self->ulen_ ? n : self->ulen_;
        while (k-- > 0)
            *t++ = self->utf8_[self->uidx_++];
        k = t - s;
        n -= k;
        if (n == 0)
        {
            self->ulen_ -= (unsigned short) (k);
            if (self->size_ >= k)
                self->size_ -= k;
            return k;
        }
        self->ulen_ = 0;
    }
    unsigned char buf[4];
    switch (self->encoding_)
    {
        case ENCODING_utf16be:
            while (n > 0 && fread(buf, 2, 1, self->file_) == 1)
            {
                int c = buf[0] << 8 | buf[1];
                if (c < 0x80)
                {
                    *t++ = (char) (c);
                    --n;
                }
                else
                {
                    if (c >= 0xD800 && c < 0xE000)
                    {
                        // UTF-16 surrogate pair
                        if (c < 0xDC00 && fread(buf + 2, 2, 1, self->file_) == 1 && (buf[2] & 0xFC) == 0xDC)
                            c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] << 8 | buf[3]);
                        else
                            c = REFLEX_NONCHAR;
                    }
                    size_t l = utf8(c, self->utf8_);
                    if (n < l)
                    {
                        memcpy(t, self->utf8_, n);
                        self->uidx_ = (unsigned short) (n);
                        self->ulen_ = (unsigned short) (l);
                        t += n;
                        n = 0;
                    }
                    else
                    {
                        memcpy(t, self->utf8_, l);
                        t += l;
                        n -= l;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        case ENCODING_utf16le:
            while (n > 0 && fread(buf, 2, 1, self->file_) == 1)
            {
                int c = buf[0] | buf[1] << 8;
                if (c < 0x80)
                {
                    *t++ = (char) (c);
                    --n;
                }
                else
                {
                    if (c >= 0xD800 && c < 0xE000)
                    {
                        // UTF-16 surrogate pair
                        if (c < 0xDC00 && fread(buf + 2, 2, 1, self->file_) == 1 && (buf[3] & 0xFC) == 0xDC)
                            c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] | buf[3] << 8);
                        else
                            c = REFLEX_NONCHAR;
                    }
                    size_t l = utf8(c, self->utf8_);
                    if (n < l)
                    {
                        memcpy(t, self->utf8_, n);
                        self->uidx_ = (unsigned short) (n);
                        self->ulen_ = (unsigned short) (l);
                        t += n;
                        n = 0;
                    }
                    else
                    {
                        memcpy(t, self->utf8_, l);
                        t += l;
                        n -= l;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        case ENCODING_utf32be:
            while (n > 0 && fread(buf, 4, 1, self->file_) == 1)
            {
                int c = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
                if (c < 0x80)
                {
                    *t++ = (char) (c);
                    --n;
                }
                else
                {
                    size_t l = utf8(c, self->utf8_);
                    if (n < l)
                    {
                        memcpy(t, self->utf8_, n);
                        self->uidx_ = (unsigned short) (n);
                        self->ulen_ = (unsigned short) (l);
                        t += n;
                        n = 0;
                    }
                    else
                    {
                        memcpy(t, self->utf8_, l);
                        t += l;
                        n -= l;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        case ENCODING_utf32le:
            while (n > 0 && fread(buf, 4, 1, self->file_) == 1)
            {
                int c = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
                if (c < 0x80)
                {
                    *t++ = (char) (c);
                    --n;
                }
                else
                {
                    size_t l = utf8(c, self->utf8_);
                    if (n < l)
                    {
                        memcpy(t, self->utf8_, n);
                        self->uidx_ = (unsigned short) (n);
                        self->ulen_ = (unsigned short) (l);
                        t += n;
                        n = 0;
                    }
                    else
                    {
                        memcpy(t, self->utf8_, l);
                        t += l;
                        n -= l;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        case ENCODING_latin:
            while (n > 0 && fread(t, 1, 1, self->file_) == 1)
            {
                int c = (unsigned char) (*t);
                if (c < 0x80)
                {
                    t++;
                    --n;
                }
                else
                {
                    utf8(c, self->utf8_);
                    *t++ = self->utf8_[0];
                    --n;
                    if (n > 0)
                    {
                        *t++ = self->utf8_[1];
                        --n;
                    }
                    else
                    {
                        self->uidx_ = 1;
                        self->ulen_ = 2;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        case ENCODING_cp437:
        case ENCODING_cp850:
        case ENCODING_cp858:
        case ENCODING_ebcdic:
        case ENCODING_cp1250:
        case ENCODING_cp1251:
        case ENCODING_cp1252:
        case ENCODING_cp1253:
        case ENCODING_cp1254:
        case ENCODING_cp1255:
        case ENCODING_cp1256:
        case ENCODING_cp1257:
        case ENCODING_cp1258:
        case ENCODING_iso8859_2:
        case ENCODING_iso8859_3:
        case ENCODING_iso8859_4:
        case ENCODING_iso8859_5:
        case ENCODING_iso8859_6:
        case ENCODING_iso8859_7:
        case ENCODING_iso8859_8:
        case ENCODING_iso8859_9:
        case ENCODING_iso8859_10:
        case ENCODING_iso8859_11:
        case ENCODING_iso8859_13:
        case ENCODING_iso8859_14:
        case ENCODING_iso8859_15:
        case ENCODING_iso8859_16:
        case ENCODING_macroman:
        case ENCODING_koi8_r:
        case ENCODING_koi8_u:
        case ENCODING_koi8_ru:
        case ENCODING_custom:
            while (n > 0 && fread(t, 1, 1, self->file_) == 1)
            {
                int c = self->page_[(unsigned char) (*t)];
                if (c < 0x80)
                {
                    *t++ = (char) (c);
                    --n;
                }
                else
                {
                    size_t l = utf8(c, self->utf8_);
                    if (n < l)
                    {
                        memcpy(t, self->utf8_, n);
                        self->uidx_ = (unsigned short) (n);
                        self->ulen_ = (unsigned short) (l);
                        t += n;
                        n = 0;
                    }
                    else
                    {
                        memcpy(t, self->utf8_, l);
                        t += l;
                        n -= l;
                    }
                }
            }
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
        default:
            t += fread(t, 1, n, self->file_);
            if (self->size_ + s >= t)
                self->size_ -= t - s;
            return t - s;
    }
}

size_t input_get(NeoastInput* self, char* s, size_t n)
{
    switch (self->type)
    {
        case NEOAST_INPUT_BUFFER:
        {
            size_t k = self->impl_.buffer_.size_;
            if (k > n)
                k = n;
            memcpy(s, self->impl_.buffer_.cstring_, k);
            self->impl_.buffer_.cstring_ += k;
            self->impl_.buffer_.size_ -= k;
            return k;
        }
        case NEOAST_INPUT_FILE:
            return file_get(&self->impl_.file_, s, n);
        case NEOAST_INPUT_CUSTOM:
            return self->impl_.custom_.get(self, s, n);
        default:
        case NEOAST_INPUT_UNK:
            assert(0 && "Unknown input type");
            return 0;
    }
}

NeoastInput* input_new_from_file(FILE* fp)
{
    return input_new_from_file_and_encoding(fp, ENCODING_plain);
}

static void input_file_init(struct FileHandle* this, file_encoding_type_t enc)
{
    struct stat st;
    if (fstat(fileno(this->file_), &st) == 0 && S_ISREG(st.st_mode) && st.st_size <= 4294967295LL)
        this->size_ = (size_t)(st.st_size);

    // assume plain (ASCII, binary or UTF-8 without BOM) content by default
    this->encoding_ = ENCODING_plain;
    (void) enc;
    while (TRUE)
    {
        // check first UTF BOM byte
        if (fread(this->utf8_, 1, 1, this->file_) == 1)
        {
            this->ulen_ = 1;
            if (this->utf8_[0] == '\0' || this->utf8_[0] == '\xef' || this->utf8_[0] == '\xfe' || this->utf8_[0] == '\xff')
            {
                // check second UTF BOM byte
                if (fread(this->utf8_ + 1, 1, 1, this->file_) == 1)
                {
                    this->ulen_ = 2;
                    if (this->utf8_[0] == '\0' && this->utf8_[1] == '\0')  // UTF-32 big endian BOM 0000XXXX?
                    {
                        if (fread(&this->utf8_[2], 2, 1, this->file_) == 1)
                        {
                            this->ulen_ = 4;
                            if (this->utf8_[2] == '\xfe' && this->utf8_[3] == '\xff') // UTF-32 big endian BOM 0000FEFF?
                            {
                                this->size_ = 0;
                                this->ulen_ = 0;
                                this->encoding_ = ENCODING_utf32be;
                            }
                        }
                    }
                    else if (this->utf8_[0] == '\xfe' && this->utf8_[1] == '\xff') // UTF-16 big endian BOM FEFF?
                    {
                        this->size_ = 0;
                        this->ulen_ = 0;
                        this->encoding_ = ENCODING_utf16be;
                    }
                    else if (this->utf8_[0] == '\xff' && this->utf8_[1] == '\xfe') // UTF-16 or UTF-32 little endian BOM FFFEXXXX?
                    {
                        if (fread(&this->utf8_[2], 2, 1, this->file_) == 1)
                        {
                            this->size_ = 0;
                            if (this->utf8_[2] == '\0' && this->utf8_[3] == '\0') // UTF-32 little endian BOM FFFE0000?
                            {
                                this->ulen_ = 0;
                                this->encoding_ = ENCODING_utf32le;
                            }
                            else
                            {
                                int c = (unsigned char)(this->utf8_[2]) | (unsigned char)(this->utf8_[3]) << 8;
                                if (c < 0x80)
                                {
                                    this->uidx_ = 2;
                                    this->ulen_ = 1;
                                }
                                else
                                {
                                    if (c >= 0xD800 && c < 0xE000)
                                    {
                                        // UTF-16 surrogate pair
                                        if (c < 0xDC00 && fread(this->utf8_, 2, 1, this->file_) == 1 && ((unsigned char)(this->utf8_[1]) & 0xFC) == 0xDC)
                                            c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + ((unsigned char)(this->utf8_[0]) | (unsigned char)(this->utf8_[1]) << 8);
                                        else
                                            c = REFLEX_NONCHAR;
                                    }
                                    this->ulen_ = (unsigned short)(utf8(c, this->utf8_)); // always a short unsigned int
                                }
                                this->encoding_ = ENCODING_utf16le;
                            }
                        }
                    }
                    else if (this->utf8_[0] == '\xef' && this->utf8_[1] == '\xbb') // UTF-8 BOM EFBBXX?
                    {
                        if (fread(&this->utf8_[2], 1, 1, this->file_) == 1)
                        {
                            this->ulen_ = 3;
                            if (this->utf8_[2] == '\xbf') // UTF-8 BOM EFBBBF?
                            {
                                if (this->size_ >= 3)
                                    this->size_ -= 3;
                                this->ulen_ = 0;
                                this->encoding_ = ENCODING_utf8;
                            }
                        }
                    }
                }
            }
            break;
        }

        break;
    }
}

NeoastInput* input_new_from_file_and_encoding(FILE* fp, file_encoding_type_t encoding)
{
    NeoastInput* self = malloc(sizeof(NeoastInput));
    self->type = NEOAST_INPUT_FILE;
    struct FileHandle* this = &self->impl_.file_;
    memset(this->utf8_, 0, sizeof(this->utf8_));
    this->uidx_ = 0;
    this->ulen_ = 0;
    this->page_ = NULL;
    this->file_ = fp;
    input_file_init(this, encoding);
    return self;
}

NeoastInput* input_new_from_buffer(const char* str, size_t len)
{
    NeoastInput* self = malloc(sizeof(NeoastInput));
    self->impl_.buffer_.cstring_ = str;
    self->impl_.buffer_.size_ = len;
    self->type = NEOAST_INPUT_BUFFER;
    return self;
}

NeoastInput* input_new_from_custom(void* ptr, neoast_input_get get)
{
    NeoastInput* self = malloc(sizeof(NeoastInput));
    self->impl_.custom_.ptr = ptr;
    self->impl_.custom_.get = get;
    self->type = NEOAST_INPUT_CUSTOM;
    return self;
}

void input_free(NeoastInput* self)
{
    switch(self->type)
    {
        case NEOAST_INPUT_BUFFER:
        case NEOAST_INPUT_CUSTOM:
        case NEOAST_INPUT_FILE:
            break;
        default:
        case NEOAST_INPUT_UNK:
            assert(0 && "Unknown input type");
            break;
    }

    free(self);
}

