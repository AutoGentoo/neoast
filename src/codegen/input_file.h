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

#ifndef NEOAST_INPUT_FILE_H
#define NEOAST_INPUT_FILE_H

#include "codegen.h"

#include <vector>
#include <memory>
#include <string>
#include <sstream>

struct InputFile
{
    File* file;
    std::string full_path;
    std::unique_ptr<char[]> contents;
    std::vector<size_t> line_starts;
    std::vector<size_t> line_sizes;

    uint32_t warn_n;
    uint32_t err_n;
    std::stringstream warning_stream;
    std::stringstream error_stream;

    explicit InputFile(const std::string& file_path);

    ~InputFile()
    {
        file_free(file);
    }

    uint32_t put_errors() const;

    const char* get_line(uint32_t lineno, size_t &len) const;
    void put_position(std::ostream& os, const TokenPosition* position) const;
    void emit_error(const TokenPosition* p, const char* format, ...);
    void emit_error(const TokenPosition* p, const char* format, va_list args);
    void emit_warning(const TokenPosition* p, const char* format, ...);
    void emit_warning(const TokenPosition* p, const char* format, va_list args);
};

#endif //NEOAST_INPUT_FILE_H
