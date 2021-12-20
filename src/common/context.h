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

struct Context
{
    virtual uint32_t has_errors() const = 0;
    virtual void emit_error_message(const TokenPosition* p, const char* format, ...) = 0;
    virtual void emit_error(const TokenPosition* p, const char* format, ...) = 0;
    virtual void emit_warning_message(const TokenPosition* p, const char* format, ...) = 0;
    virtual void emit_warning(const TokenPosition* p, const char* format, ...) = 0;
};

#endif //NEOAST_CONTEXT_H
