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

#ifdef __cplusplus
extern "C" {
#endif

#include <neoast.h>

void dump_table(const uint32_t* table, const void* cc,
                const char** tok_names, uint8_t state_wrap, FILE* fp,
                const char* indent_str);

void dump_graphviz(const void* cc, FILE* fp);

#ifdef __cplusplus
}

#include <ostream>
#include <memory>
#include <parsergen/canonical_collection.h>

class Exception : std::exception
{
    std::string what_;

public:
    explicit Exception(std::string what) : what_(std::move(what)) {}
    const char * what() const noexcept override { return what_.c_str(); }
};

template<typename T_,
        typename... Args>
std::string variadic_string(const char* format,
                            T_ arg1,
                            Args... args)
{
    int size = std::snprintf(nullptr, 0, format, arg1, args...);
    if(size <= 0)
    {
        throw Exception( "Error during formatting: " + std::string(format));
    }
    size += 1;

    auto buf = std::unique_ptr<char[]>(new char[size]);
    std::snprintf(buf.get(), size, format, arg1, args...);

    return {buf.get(), buf.get() + size - 1}; // We don't want the '\0' inside
}

void dump_table_cxx(
        const uint32_t* table, const parsergen::CanonicalCollection* cc,
        const char** tok_names, uint8_t state_wrap,
        std::ostream& os, const char* indent_str);

void dump_graphviz_cxx(
        const parsergen::CanonicalCollection* cc,
        std::ostream& os,
        bool full = true);

#endif

#endif //NEOAST_UTIL_H
