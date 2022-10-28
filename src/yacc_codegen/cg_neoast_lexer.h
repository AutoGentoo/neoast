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

#ifndef NEOAST_CG_NEOAST_LEXER_H
#define NEOAST_CG_NEOAST_LEXER_H

#include "cg_lexer.h"
#include "cg_util.h"
#include "regex.h"
#include <yacc_codegen/codegen_priv.h>

struct CGNeoastLexerImpl;
class CGNeoastLexer : public CGLexer
{
    CGNeoastLexerImpl* impl_;

public:
    explicit CGNeoastLexer(const File* self, const MacroEngine &m_engine_,
                           const Options &options);

    void put_top(std::ostream &os) const override;
    void put_global(std::ostream &os) const override;
    void put_bottom(std::ostream& os) const override;
    const Options& get_options() const;
    std::string get_init() const override;
    std::string get_delete() const override;
    ~CGNeoastLexer();

    // Internal call names TODO(tumbar)
    std::string get_new_inst(const std::string &name) const override;
    std::string get_del_inst(const std::string& name) const override;
    std::string get_ll_next(const std::string& name) const override;
};
#endif //NEOAST_CG_NEOAST_LEXER_H
