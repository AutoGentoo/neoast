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

#include <common/context.h>

class InputFile : public Context
{
    File* file;
    size_t file_length;
    std::string full_path;
    std::unique_ptr<char[]> contents;
    std::vector<size_t> line_starts;
    std::vector<size_t> line_sizes;

    std::stringstream message_stream;

    const char* get_line(uint32_t lineno, size_t &len) const;
    void put_position(std::ostream& os, const TokenPosition* position) const;
    void emit_message_typed(Context::Message msg, const TokenPosition *p, const char *format, va_list args) override;

public:
    explicit InputFile(const std::string& file_path);

    ~InputFile()
    {
        if (file) file_free(file);
    }

    uint32_t put_errors() const;

    inline const File* get() const { return file; }
    inline const std::string& get_path() const { return full_path; }
};

#endif //NEOAST_INPUT_FILE_H
