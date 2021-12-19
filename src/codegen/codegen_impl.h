//
// Created by tumbar on 7/17/21.
//

#ifndef NEOAST_CODEGEN_IMPL_H
#define NEOAST_CODEGEN_IMPL_H

#include <codegen/codegen.h>
#include <codegen/codegen_priv.h>
#include "cg_grammar.h"
#include "cg_lexer.h"

struct CodeGenImpl
{
    CodeGen* parent;

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

    std::map <std::string, up<Code>> destructors;
    Options options;
    uint32_t ascii_mappings[NEOAST_ASCII_MAX] = {0};
    std::map<int, int> precedence_mapping;

    explicit CodeGenImpl(CodeGen* parent_);
    sp<CGToken> get_token(const std::string &name) const;
    void parse(const File* self);

    void write_header(std::ostream &os) const;
    void write_source(std::ostream &os) const;

private:
    void parse_header(const File* self);
    void parse_lexer(const File* self);
    void parse_grammar(const File* self);

    void put_ascii_mappings(std::ostream &os) const;
    void put_parsing_table(std::ostream &os) const;
    void put_table_debug(std::ostream& os,
                         const uint32_t* table,
                         const parsergen::CanonicalCollection* cc) const;
    void register_action(const sp<CGAction>& ptr);
    void register_grammar(const sp<CGGrammarToken>& ptr);
};

#endif //NEOAST_CODEGEN_IMPL_H
