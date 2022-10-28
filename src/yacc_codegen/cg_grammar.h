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

#ifndef NEOAST_CG_GRAMMAR_H
#define NEOAST_CG_GRAMMAR_H

#include <yacc_codegen/codegen.h>
#include <yacc_codegen/codegen_priv.h>

struct CGGrammarsImpl;

class CGGrammars
{
    CGGrammarsImpl* impl_;
public:
    CGGrammars(const CodeGen* cg, const File* self);

    void put_table(std::ostream &os) const;
    void put_rules(std::ostream &os) const;
    void put_actions(std::ostream &os) const;

    uint32_t size() const;
    GrammarRule* get() const;
    const TokenPosition** get_positions() const;

    ~CGGrammars();
};

#endif //NEOAST_CG_GRAMMAR_H
