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


#ifndef NEOAST_CODEGEN_IMPL_H
#define NEOAST_CODEGEN_IMPL_H

#include <yacc_codegen/codegen.h>
#include <yacc_codegen/codegen_priv.h>
#include "cg_grammar.h"
#include "cg_lexer.h"
#include "regex.h"
#include "input_file.h"

struct CodeGenImpl
{
    CodeGen* parent;
    InputFile* input;

    MacroEngine m_engine;

    up <Code> top;
    up <Code> bottom;
    up <Code> union_;
    up <Code> include_;

    std::string start_type;
    std::string start_token;

    sp <CGLexer> lexer;
    sp <CGGrammars> grammar;

    std::vector <sp<CGAction>> action_tokens;
    std::vector <sp<CGGrammarToken>> grammar_tokens;
    std::vector <std::string> tokens;  // identifier names (goes into ENUM)
    std::vector <std::string> tokens_names; // debug names (includes ascii characters)

    // Generated parser
    up<const char* []> token_names_ptr;
    up<GrammarParser> parser;
    up<parsergen::CanonicalCollection> cc;
    up<uint8_t[]> precedence_table;
    up<uint32_t[]> parsing_table;

    std::map <std::string, up<Code>> destructors;
    Options options;
    uint32_t ascii_mappings[NEOAST_ASCII_MAX] = {0};
    std::map<int, int> precedence_mapping;

    explicit CodeGenImpl(CodeGen* parent_, InputFile* input_file);
    sp<CGToken> get_token(const std::string &name) const;
    void parse();

    void write_header(std::ostream &os, bool dump_license=true) const;
    void write_source(std::ostream &os, bool dump_license=true) const;

private:
    void parse_header(const File* self);
    void parse_lexer(const File* self);
    void parse_grammar(const File* self);
    void init_cc();

    void put_ascii_mappings(std::ostream &os) const;
    void put_parsing_table(std::ostream &os) const;
    void register_action(const sp<CGAction>& ptr);
    void register_grammar(const sp<CGGrammarToken>& ptr);
};

#endif //NEOAST_CODEGEN_IMPL_H
