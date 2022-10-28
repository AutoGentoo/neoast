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

#ifndef NEOAST_CONTEXT_H
#define NEOAST_CONTEXT_H

#include <cstdarg>

#ifdef __cplusplus
struct Ast
{
    virtual ~Ast() = default;
protected:
    Ast() = default;
};
#endif

struct Context
{
protected:
    enum Message
    {
        NOTE,
        MESSAGE,
        WARNING,
        ERROR,
    };

    virtual void emit_message_typed(Message msg, const TokenPosition* p, const char* format, va_list args) = 0;

    uint32_t error_n = 0;
    uint32_t warning_n = 0;

public:
    virtual uint32_t has_errors() const { return error_n; };

#define FWD_VA_LIST(type_) { va_list args; va_start(args, format); emit_message_typed((type_), p, format, args); va_end(args); }
#define FWD_VA_LIST_INC(type_, inc_) { FWD_VA_LIST(type_) (inc_)++; }

    void emit_message(const TokenPosition* p, const char* format, ...) FWD_VA_LIST(MESSAGE);
    void emit_error(const TokenPosition* p, const char* format, ...) FWD_VA_LIST_INC(ERROR, error_n);
    void emit_warning(const TokenPosition* p, const char* format, ...) FWD_VA_LIST_INC(WARNING, warning_n);
    void emit_note(const TokenPosition* p, const char* format, ...) FWD_VA_LIST(NOTE);

#undef FWD_VA_LIST

};

#endif //NEOAST_CONTEXT_H
