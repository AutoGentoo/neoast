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

#ifndef NEOAST_INPUT_H
#define NEOAST_INPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && defined NEOAST_WITH_CPLUSPLUS
#include <istream>
#endif

typedef enum
{
    NEOAST_INPUT_UNK,
    NEOAST_INPUT_BUFFER,
    NEOAST_INPUT_FILE,
#if defined(__cplusplus) && defined NEOAST_WITH_CPLUSPLUS
    NEOAST_INPUT_ISTREAM,
#endif
} input_t;

typedef enum
{
    ENCODING_plain = 0, ///< plain octets: 7-bit ASCII, 8-bit binary or UTF-8 without BOM detected
    ENCODING_utf8 = 1, ///< UTF-8 with BOM detected
    ENCODING_utf16be = 2, ///< UTF-16 big endian
    ENCODING_utf16le = 3, ///< UTF-16 little endian
    ENCODING_utf32be = 4, ///< UTF-32 big endian
    ENCODING_utf32le = 5, ///< UTF-32 little endian
    ENCODING_latin = 6, ///< ISO-8859-1, Latin-1
    ENCODING_cp437 = 7, ///< DOS CP 437
    ENCODING_cp850 = 8, ///< DOS CP 850
    ENCODING_cp858 = 9, ///< DOS CP 858
    ENCODING_ebcdic = 10, ///< EBCDIC
    ENCODING_cp1250 = 11, ///< Windows CP 1250
    ENCODING_cp1251 = 12, ///< Windows CP 1251
    ENCODING_cp1252 = 13, ///< Windows CP 1252
    ENCODING_cp1253 = 14, ///< Windows CP 1253
    ENCODING_cp1254 = 15, ///< Windows CP 1254
    ENCODING_cp1255 = 16, ///< Windows CP 1255
    ENCODING_cp1256 = 17, ///< Windows CP 1256
    ENCODING_cp1257 = 18, ///< Windows CP 1257
    ENCODING_cp1258 = 19, ///< Windows CP 1258
    ENCODING_iso8859_2 = 20, ///< ISO-8859-2, Latin-2
    ENCODING_iso8859_3 = 21, ///< ISO-8859-3, Latin-3
    ENCODING_iso8859_4 = 22, ///< ISO-8859-4, Latin-4
    ENCODING_iso8859_5 = 23, ///< ISO-8859-5, Cyrillic
    ENCODING_iso8859_6 = 24, ///< ISO-8859-6, Arabic
    ENCODING_iso8859_7 = 25, ///< ISO-8859-7, Greek
    ENCODING_iso8859_8 = 26, ///< ISO-8859-8, Hebrew
    ENCODING_iso8859_9 = 27, ///< ISO-8859-9, Latin-5
    ENCODING_iso8859_10 = 28, ///< ISO-8859-10, Latin-6
    ENCODING_iso8859_11 = 29, ///< ISO-8859-11, Thai
    ENCODING_iso8859_13 = 30, ///< ISO-8859-13, Latin-7
    ENCODING_iso8859_14 = 31, ///< ISO-8859-14, Latin-8
    ENCODING_iso8859_15 = 32, ///< ISO-8859-15, Latin-9
    ENCODING_iso8859_16 = 33, ///< ISO-8859-16
    ENCODING_macroman = 34, ///< Macintosh Roman with CR to LF translation
    ENCODING_koi8_r = 35, ///< KOI8-R
    ENCODING_koi8_u = 36, ///< KOI8-U
    ENCODING_koi8_ru = 37, ///< KOI8-RU
    ENCODING_custom = 38, ///< custom code page
} file_encoding_type_t;

struct NeoastInput_prv
{
    input_t type;
    union
    {
        struct BufferHandle
        {
            const char* cstring_;
            size_t size_;
        } buffer_;
        struct FileHandle
        {
            FILE* file_;
            file_encoding_type_t encoding_;
            size_t size_;    ///< size of the remaining input in bytes (size_ == 0 may indicate size is not set)
            char utf8_[8]; ///< UTF-8 normalization buffer, >=8 bytes
            unsigned short uidx_;    ///< index in utf8_[]
            unsigned short ulen_;    ///< length of data in utf8_[] or 0 if no data
            const unsigned short* page_;    ///< custom code page
        } file_;
#if defined(__cplusplus) && defined NEOAST_WITH_CPLUSPLUS
        std::istream* ios_;
#endif
    } impl_;
};

/**
 * Get n bytes from the input stream
 * @param self input stream to read from
 * @param s target buffer to fill
 * @param n number of bytes to read at maximum
 * @return actual number of bytes read
 */
size_t input_get(NeoastInput* self, char* s, size_t n);

/**
 * Create an input stream from a C string pointer
 * @param str pointer to string
 * @param len length of string
 * @return input
 */
NeoastInput* input_new_from_buffer(const char* str, size_t len);

/**
 * Create an input given a POSIX file pointer
 * @param fp POSIX file pointer
 * @param encoding encoding type to read file with
 * @return input
 */
NeoastInput* input_new_from_file_and_encoding(FILE* fp, file_encoding_type_t encoding);

/**
 * input_new_from_file_and_encoding() with default plain-text encoding
 * @param fp file pointer structure
 * @return input
 */
NeoastInput* input_new_from_file(FILE* fp);

void input_free(NeoastInput* self);

#ifdef __cplusplus
};
#endif

#endif //NEOAST_INPUT_H
