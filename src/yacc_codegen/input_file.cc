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

#include <util/util.h>
#include <iostream>
#include "input_file.h"

#define PATH_MAX (4096)
#define ERROR_CONTEXT_LINE_N 1

static InputFile* current_input_file = nullptr;

extern "C" {
uint32_t cc_init();
void cc_free();
void* cc_allocate_buffers();
void cc_free_buffers(void* self);
struct File* cc_parse_len(void* error_ctx, void* buffers, const char* input, uint64_t input_len);
}

void emit_warning(const TokenPosition* p, const char* format, ...)
{
    if (!current_input_file) return;
    va_list args;
    va_start(args, format);
    current_input_file->emit_warning(p, format, args);
    va_end(args);
}

uint32_t has_errors()
{
    return current_input_file ? current_input_file->has_errors() : 0;
}

void emit_error(const TokenPosition* p, const char* format, ...)
{
    if (!current_input_file) return;
    va_list args;
    va_start(args, format);
    current_input_file->emit_error(p, format, args);
    va_end(args);
}

void lexing_error_cb(void* ctx,
                     const char* input,
                     const TokenPosition* position,
                     const char* lexer_state)
{
    (void) lexing_error_cb;
    (void) input;

    static_cast<InputFile*>(ctx)->emit_error(position, "[state %s] Unmatched token near", lexer_state);
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
    static_cast<InputFile*>(ctx)->emit_error(position, err_os.str().c_str());
}

InputFile::InputFile(const std::string &file_path)
: file(nullptr)
{
    current_input_file = this;

    static char file_p[PATH_MAX];
    FILE* fp = fopen(file_path.c_str(), "r");
    if (!fp)
    {
        full_path = file_path;
        emit_error(nullptr, "Failed to open file");
        return;
    }

    full_path = realpath(file_path.c_str(), file_p);

    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    contents = std::unique_ptr<char[]>(new char[file_length + 1]);
    char* start = contents.get();

    // Read the file
    size_t offset = 0;
    while ((offset += fread(start + offset, 1, 1024, fp)) < file_length);

    fclose(fp);

    // null terminator
    contents[file_length] = 0;

    size_t line_start = 0;
    char* end = start + file_length;
    char* iter = start;
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

    cc_init();
    void* buf = cc_allocate_buffers();
    file = cc_parse_len(this, buf, start, file_length);
    cc_free_buffers(buf);
    cc_free();

    if (!file)
    {
        emit_error(nullptr, "Failed to parse file");
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
           << "\033[0m\n";
    }
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
            msg_text = "";
            break;
        case WARNING:
            msg_text = "\033[1;33mwarning:\033[1;0m ";
            break;
        case ERROR:
            msg_text = "\033[1;313merror:\033[1;0m ";
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

const char* InputFile::get_line(uint32_t lineno, size_t &len) const
{
    len = line_sizes.at(lineno);
    size_t start = line_starts.at(lineno);
    assert(start + len <= file_length);
    const char* out = contents.get() + start;
    return out;
}

uint32_t InputFile::put_errors() const
{
    std::cout << message_stream.str();

    if (error_n || warning_n)
    {
        std::cout << "Generated " << warning_n << " warnings and "
                  << error_n << " errors\n";
    }

    return error_n;
}
