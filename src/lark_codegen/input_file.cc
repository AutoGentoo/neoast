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

#include <neoast.h>
#include <util/util.h>
#include <iostream>
#include <fstream>
#include "input_file.h"
#include "common.h"

#include <sys/stat.h>
#include <cstring>

#define PATH_MAX (4096)
#define ERROR_CONTEXT_LINE_N 1

void lexing_error_cb(void* ctx,
                     const char* input,
                     const TokenPosition* position,
                     const char* lexer_state)
{
    (void) lexing_error_cb;
    (void) input;

    static_cast<neoast::InputFile*>(ctx)->emit_error(position, "[state %s] Unmatched token near", lexer_state);
}

void parsing_error_cb(void* ctx,
                      const char* const* token_names,
                      const TokenPosition* position,
                      uint32_t last_token,
                      uint32_t current_token,
                      const uint32_t expected_tokens[],
                      uint32_t expected_tokens_n)
{
    std::stringstream err_os;
    err_os << "expected ";

    for (int i = 0; i < expected_tokens_n; i++)
    {
        if (i > 0)
        {
            err_os << ", ";
            if (i + 1 == expected_tokens_n) err_os << "or ";
        }

        err_os << token_names[expected_tokens[i]];
    }

    err_os << " before " << token_names[current_token]
           << " (and after " << token_names[last_token] << ")";
    static_cast<neoast::InputFile*>(ctx)->emit_error(position, err_os.str().c_str());
}

namespace neoast
{
    InputFile::InputFile(const std::string &file_path)
            : warn_n(0), err_n(0), file_length(0)
    {
        struct stat buf{};
        if (stat(file_path.c_str(), &buf) != 0)
        {
            throw neoast::Exception(
                    "Failed to open file " +
                    std::string(::strerror(errno)));
        }

        std::ifstream f;
        f.open(file_path);

        file_stream << f.rdbuf();
        contents = std::move(file_stream.str());
        file_length = contents.size();

        f.close();

        static char file_p[PATH_MAX];
        full_path = realpath(file_path.c_str(), file_p);

        const char* start = contents.c_str();

        size_t line_start = 0;
        const char* end = start + file_length;
        const char* iter = start;
        while(iter < end)
        {
            if (*iter == '\n')
            {
                size_t pos = iter - start;
                line_starts.push_back(line_start);
                line_sizes.push_back(pos - line_start);
                line_start = pos + 1;
            }
            iter++;
        }
    }

    void InputFile::put_position(std::ostream &os, const TokenPosition* position) const
    {
        if (!position || position->line < 0)
        {
            return;
        }

        // Get the number of digits in the line number:
        int dig_n = snprintf(nullptr, 0, "%+d", position->line);

        for (int i = (int) position->line - ERROR_CONTEXT_LINE_N; i < (int) position->line; i++)
        {
            if (i < 0)
            {
                continue;
            }

            size_t l_len;
            const char* line = get_line(i, l_len);
            os << variadic_string("% *d | %.*s\n", dig_n, i + 1,
                                  l_len, line);
        }

        if (position->line > 0)
        {
            size_t len = position->len ? position->len - 1 : position->len;
            os << std::string(position->col + 3 + dig_n, ' ')
               << "\033[1;32m^"
               << std::string(len, '~')
               << "\033[1;0m\n";
        }
    }

    const char* InputFile::get_line(uint32_t lineno, size_t &len) const
    {
        len = line_sizes.at(lineno);
        size_t start = line_starts.at(lineno);
        assert(start + len <= file_length);
        const char* out = get_content() + start;
        return out;
    }

    void InputFile::emit_message_typed(Context::Message msg, const TokenPosition* p, const char* format, va_list args)
    {
        const char* msg_text;
        switch(msg)
        {
            case NOTE:
                msg_text = "\033[1;36mnote:\033[1;0m ";
                break;
            case MESSAGE:
                msg_text = "\033[1;0m";
                break;
            case WARNING:
                msg_text = "\033[1;33mwarning:\033[1;0m ";
                break;
            case ERROR:
                msg_text = "\033[1;31merror:\033[1;0m ";
                break;
        }

        if (p)
        {
            message_stream
                    << variadic_string(
                            "\033[1m%s:%d:%d: %s",
                            full_path.c_str(), p->line, p->col + 1,
                            msg_text)
                    << variadic_string(format, args)
                    << "\n";
            put_position(message_stream, p);
        }
        else
        {
            message_stream
                    << variadic_string(
                            "\033[1m%s: %s",
                            full_path.c_str(),
                            msg_text)
                    << variadic_string(format, args)
                    << "\n";
        }
    }

    const std::stringstream &InputFile::get_messages() const
    {
        return message_stream;
    }

    void InputFile::clear_messages()
    {
        message_stream.str("");
    }

    InputFile &GlobalContext::get(int ctx_id)
    {
        if (ctx_id >= files.size())
        {
            assert(!files.empty());
            return files.at(0);
        }

        return files.at(ctx_id);
    }

    void GlobalContext::emit_message_typed(Context::Message msg, const TokenPosition* p, const char* format, va_list args)
    {
        get(p ? p->context : 0).emit_message_typed(msg, p, format, args);
    }

    GlobalContext::GlobalContext(std::vector<InputFile> &files_)
            : files(files_)
    {
    }

    void GlobalContext::put_messages()
    {
        for (auto& file : files)
        {
            std::cout << file.get_messages().str();
            file.clear_messages();
        }
    }

    uint32_t GlobalContext::has_errors() const
    {
        uint32_t error_n = 0;
        for (const auto& iter : files)
        {
            error_n += iter.has_errors();
        }
        return error_n;
    }
}
