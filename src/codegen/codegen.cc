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


#include <memory>
#include <string>
#include "codegen_impl.h"
#include <parsergen/canonical_collection.h>
#include <util/util.h>
#include <utility>


std::string grammar_filename;
const TokenPosition NO_POSITION = {0, 0};


void CodeGenImpl::parse_header(const File* self)
{
    int action_id = 0;  // ID = 0 reserved for TOK_EOF
    // Setup all tokens and grammar rules as well as
    // other header information
    action_tokens.emplace_back(new CGAction(nullptr, "EOF", action_id++));

    std::map<key_val_t, std::pair<const char*, up<Code>*>>
            single_appearance_map{
            {KEY_VAL_TOP,    {"top",    &top}},
            {KEY_VAL_BOTTOM, {"bottom", &bottom}},
            {KEY_VAL_UNION,  {"union",  &union_}},
            {KEY_VAL_LEXER,  {"lexer",  &lexer_input}},
    };

    // Run round 1 of iterations
    // Lowest depends first
    for (KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_TOP:
            case KEY_VAL_BOTTOM:
            case KEY_VAL_UNION:
            case KEY_VAL_LEXER:
            {
                auto &ptr = single_appearance_map[iter->type];

                if (*ptr.second)
                {
                    emit_error(&iter->position, "cannot have multiple %%%s definitions",
                               ptr.first);
                    break;
                }

                *ptr.second = std::make_unique<Code>(iter);
            }
                break;
            case KEY_VAL_TOKEN:
            case KEY_VAL_TOKEN_ASCII:
                register_action(std::make_shared<CGAction>(
                        iter, action_id++,
                        iter->type == KEY_VAL_TOKEN_ASCII));
                break;
            case KEY_VAL_TOKEN_TYPE:
                register_action(std::make_shared<CGTypedAction>(iter, action_id++));
                break;
            case KEY_VAL_TYPE:
                // We don't know how many action tokens there are yet
                // Initialize the IDs later
                register_grammar(std::make_shared<CGGrammarToken>(iter, 0));
                break;
            case KEY_VAL_OPTION:
                options.handle(iter);
                break;
            default:
                break;
        }
    }

    // Run round 2 of iterations
    // Depends on results from round 1
    for (KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
            {
                auto t = get_token(iter->value);
                if (!t)
                {
                    emit_error(&iter->position, "Undefined token");
                }
                else if (!dynamic_cast<CGAction*>(t.get()))
                {
                    emit_error(&iter->position, "Cannot use %%type token in precedence");
                }
                else if (precedence_mapping.find(t->id) != precedence_mapping.end())
                {
                    emit_error(&iter->position, "Already defined precedence for '%s'", t->name.c_str());
                }
                else
                {
                    precedence_mapping.emplace(
                            t->id,
                            iter->type == KEY_VAL_LEFT ? PRECEDENCE_LEFT : PRECEDENCE_RIGHT);
                }
            }
                break;
            case KEY_VAL_MACRO:
                m_engine.add(iter->key, iter->value);
                break;
            case KEY_VAL_START:
                if (!start_type.empty() || !start_token.empty())
                {
                    emit_warning(&iter->position, "Using %%start multiple times will override previous uses");
                }

                start_type = iter->key;
                start_token = iter->value;
                break;
            case KEY_VAL_DESTRUCTOR:
            {
                if (destructors.find(iter->key) != destructors.end())
                {
                    emit_error(&iter->position, "Destructor for type '%s' is already defined",
                               iter->key);
                }

                destructors[iter->key] = std::make_unique<Code>(iter);
            }
                break;
            default:
                break;
        }
    }

    // Add the augment token with the start type
    register_grammar(std::make_shared<CGGrammarToken>(nullptr, start_type, "TOK_AUGMENT", 0));

    // All grammar tokens are initialized with their true ids
    // We need to do this here because only now do we know how many
    // action tokens there are.
    int grammar_i = 0;
    for (auto &grammar_tok : grammar_tokens)
    {
        assert(!grammar_tok->id);
        grammar_tok->id = action_id + grammar_i++;
    }

    for (const auto &i : action_tokens)
    { tokens.push_back(i->name); }
    for (const auto &i : grammar_tokens)
    { tokens.push_back(i->name); }
}

void CodeGenImpl::parse_lexer(const File* self)
{
    if (!(self->lexer_rules || !options.lexer_file.empty() || lexer_input))
    {
        emit_error(nullptr,
                   "No lexer input found\n"
                   "Use %lexer {}, %option lexer_file=\"PATH\", or builtin lexer");
        return;
    }

    if (self->lexer_rules)
    {
        if (!options.lexer_file.empty() || lexer_input)
        {
            emit_error(nullptr,
                       "Multiple lexer input types defined\n"
                       "Use %lexer {}, %option lexer_file=\"PATH\", or builtin lexer");
            return;
        }

        // Using the builtin lexer
        if (!options.no_warn_builtin)
        {
            emit_warning(&self->lexer_rules->position,
                         "BUILTIN lexer should only be used for "
                         "bootstrap compiler-compiler");
        }

        lexer = std::shared_ptr<CGBuiltinLexer>(new CGBuiltinLexer(self, m_engine, options));
    }
    else if (lexer_input)
    {
        if (!options.lexer_file.empty())
        {
            emit_error(nullptr,
                       "Multiple lexer input types defined\n"
                       "Use %lexer {}, %option lexer_file=\"PATH\", or builtin lexer");
            return;
        }

        // Use the full Reflex lexer generator

    }
    else
    {
        // Manual lexer file input
    }
}

void CodeGenImpl::parse_grammar(const File* self)
{
    if (start_type.empty() || start_token.empty())
    {
        emit_error(nullptr, "Undeclared %%start type");
        return;
    }

    grammar = std::shared_ptr<CGGrammars>(new CGGrammars(parent, self));
}

void CodeGenImpl::put_ascii_mappings(std::ostream &os) const
{
    int i = 0;
    for (int row = 0; i < NEOAST_ASCII_MAX; row++)
    {
        os << "        ";
        for (int col = 0; col < 6 && i < NEOAST_ASCII_MAX; col++, i++)
        {
            os << variadic_string("0x%03X,%c", ascii_mappings[i], (col + 1 >= 6) ? '\n' : ' ');
        }
    }
}

void CodeGenImpl::put_parsing_table(std::ostream &os) const
{
    up<const char* []> token_names_ptr(new const char* [tokens.size()]);
    int i = 0;
    for (const auto &n : tokens)
    { token_names_ptr[i++] = n.c_str(); }

    const GrammarParser parser{
            .ascii_mappings = ascii_mappings,
            .grammar_rules = grammar->get(),
            .token_names = token_names_ptr.get(),
            .grammar_n = grammar->size(),
            .token_n = static_cast<uint32_t>(tokens.size()) - 1,
            .action_token_n = static_cast<uint32_t>(action_tokens.size()),
    };

    CanonicalCollection* cc = canonical_collection_init(&parser);
    canonical_collection_resolve(cc, options.parser_type);

    uint8_t error;
    up<uint8_t[]> precedence_table(new uint8_t[tokens.size()]);
    for (const auto &mapping : precedence_mapping)
    {
        precedence_table[mapping.first] = mapping.second;
    }

    uint32_t* parsing_table = canonical_collection_generate(cc, precedence_table.get(), &error);

    if (error)
    {
        emit_error(nullptr, "Failed to generate parsing table");
        return;
    }

    if (options.debug_table)
    {
        put_table_debug(os, parsing_table, cc);
    }

    // Actually put the LR parsing table
    i = 0;
    os << "static const\nuint32_t " << options.prefix << "_parsing_table[] = {\n";
    for (int state_i = 0; state_i < cc->state_n; state_i++)
    {
        os << "        ";
        for (int tok_i = 0; tok_i < cc->parser->token_n; tok_i++, i++)
        {
            os << variadic_string("0x%08X,%c", parsing_table[i], tok_i + 1 >= cc->parser->token_n ? '\n' : ' ');
        }
    }
    os << "};";

    canonical_collection_free(cc);
    free(parsing_table);
}

void CodeGenImpl::put_table_debug(std::ostream &os,
                                  const uint32_t* table,
                                  const CanonicalCollection* cc) const
{
    os << "// Token names:\n";

    std::string fallback;
    fallback.reserve(tokens.size() + 1);
    for (int i = 0; i < tokens.size(); i++)
    {
        if (i == 0)
        {
            fallback[i] = '$';
        }
        else if (i < action_tokens.size())
        {
            fallback[i] = (char) ('a' + (char) i - 1);
        }
        else
        {
            fallback[i] = (char) ('A' + (char) i - (action_tokens.size()));
        }
    }

    const char* debug_ids = fallback.c_str();
    if (!options.debug_ids.empty())
    { debug_ids = options.debug_ids.c_str(); }

    for (int i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].substr(0, 13) == "ASCII_CHAR_0x")
        {
            os << variadic_string("//  %s => '%c' ('%c')\n",
                                  tokens[i].c_str(), debug_ids[i],
                                  get_ascii_from_name(tokens[i].c_str()));
        }
        else
        {
            os << variadic_string("//  %s => '%c'\n", tokens[i].c_str(), debug_ids[i]);
        }
    }

    os << "\n";

    // Dump_table is built to work with C FILE* because
    // it was written before CodeGen was implemented and
    // is meant to work with the unit tests.
    // We can open a memory stream and dump to our output stream

    char* ptr = nullptr;
    size_t size = 0;
    FILE* fp = open_memstream(&ptr, &size);

    dump_table(table, cc, debug_ids, 0, fp, "//  ");
    fflush(fp);
    os.write(ptr, static_cast<ssize_t>(size));
    os.flush();
    fclose(fp);
}

sp<CGToken> CodeGenImpl::get_token(const std::string &name) const
{
    for (const auto &i : action_tokens)
    {
        if (i->name == name)
        {
            return i;
        }
    }

    for (const auto &i : grammar_tokens)
    {
        if (i->name == name)
        {
            return i;
        }
    }

    return nullptr;
}

void CodeGenImpl::register_action(const sp<CGAction> &ptr)
{
    if (get_token(ptr->name) != nullptr)
    {
        emit_error(ptr.get(), "Repeated definition of token");
        return;
    }

    if (ptr->is_ascii)
    {
        ascii_mappings[get_ascii_from_name(ptr->name.c_str())] = ptr->id + NEOAST_ASCII_MAX;
    }

    action_tokens.push_back(ptr);
}

void CodeGenImpl::register_grammar(const sp<CGGrammarToken> &ptr)
{
    if (get_token(ptr->name) != nullptr)
    {
        emit_error(ptr.get(), "Repeated definition of token");
        return;
    }

    grammar_tokens.push_back(ptr);
}

CodeGenImpl::CodeGenImpl(CodeGen* parent_)
: parent(parent_),
  top(nullptr), bottom(nullptr), union_(nullptr),
  lexer_input(nullptr),
  options(), m_engine()
{
}

void CodeGenImpl::parse(const File* self)
{
    parse_header(self);
    if (has_errors())
    { return; }

    parse_lexer(self);
    if (has_errors())
    { return; }

    parse_grammar(self);
    if (has_errors())
    { return; }
}

CodeGen::CodeGen(const File* self)
        : impl_(new CodeGenImpl(this))
{
    impl_->parse(self);
}

sp<CGToken> CodeGen::get_token(const std::string &name) const
{ return impl_->get_token(name); }

const char* CodeGen::get_start_token() const
{ return impl_->start_token.c_str(); }

const std::vector<std::string> &CodeGen::get_tokens() const
{ return impl_->tokens; }

const Options &CodeGen::get_options() const
{ return impl_->options; }

void CodeGen::write_header(std::ostream &os) const
{ impl_->write_header(os); }

void CodeGen::write_source(std::ostream &os) const
{ impl_->write_source(os); }

CodeGen::~CodeGen()
{ delete impl_; }


int codegen_write(const char* grammar_file_path,
                  const File* self,
                  const char* output_file_cc,
                  const char* output_file_hh)
{
    if (grammar_file_path)
    { grammar_filename = grammar_file_path; }

    std::ofstream hh_os(output_file_hh);
    std::ofstream cc_os(output_file_cc);

    try
    {
        CodeGen c(self);
        if (has_errors())
        { return 1; }

        c.write_header(hh_os);
        if (has_errors())
        { return 1; }

        c.write_source(cc_os);
        if (has_errors())
        { return 1; }
    }
    catch (const ASTException &e)
    { emit_error(e.position(), e.what()); }
    catch (const Exception &e)
    { emit_error(nullptr, e.what()); }
    catch (const std::exception &e)
    { emit_error(nullptr, "System exception: %s", e.what()); }

    return has_errors();
}
