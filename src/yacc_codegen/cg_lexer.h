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

#ifndef NEOAST_CG_LEXER_H
#define NEOAST_CG_LEXER_H

#include <iostream>

class CGLexer
{
    /**
     * Subclasses must implement the global and init
     * functions to paste into the codegen
     */
public:
    virtual void put_top(std::ostream &os) const = 0;
    virtual void put_global(std::ostream &os) const = 0;
    virtual void put_bottom(std::ostream &os) const = 0;

    virtual std::string get_init() const = 0;
    virtual std::string get_delete() const = 0;

    /**
     * Create and destroy parsing instances of the lexer
     */
    virtual std::string get_new_inst(const std::string &name) const = 0;
    virtual std::string get_del_inst(const std::string &name) const = 0;
    virtual std::string get_ll_next(const std::string &name) const = 0;
};

#endif //NEOAST_CG_LEXER_H
