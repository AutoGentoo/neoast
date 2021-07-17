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

#include "codegen.h"
#include "regex.h"
#include "cg_util.h"
#include "cg_lexer.h"
#include <parsergen/canonical_collection.h>
#include <util/util.h>
#include <algorithm>
#include <utility>
#include "codegen_priv.h"


std::string grammar_filename;
const TokenPosition NO_POSITION = {0, 0};


class CodeGen
{
    friend CGGrammar;

    MacroEngine m_engine;

    up<Code> top;
    up<Code> bottom;
    up<Code> union_;

    std::string start_type;
    std::string start_token;

    up<Code> lexer_input;

    enum
    {
        LEXER_BUILTIN,
        LEXER_REFLEX_RAW,
        LEXER_REFLEX_FILE
    } lexer_type;

    std::vector<sp<CGAction>> action_tokens;
    std::vector<sp<CGGrammarToken>> grammar_tokens;
    std::vector<std::string> tokens;

    std::map<std::string, up<Code>> destructors;

    std::vector<up<CGLexerState>> ll_states;
    std::map<std::string, std::vector<CGGrammar>> gg_rules_cg;
    std::vector<GrammarRule> gg_rules;
    std::vector<int32_t> grammar_table;

    Options options;

    uint32_t grammar_i;
    uint32_t grammar_n;
    uint32_t ascii_mappings[NEOAST_ASCII_MAX] = {0};
    std::map<int, int> precedence_mapping;

    void parse_header(const File* self)
    {
        // Setup all tokens and grammar rules as well as
        // other header information

        std::map<key_val_t, std::pair<const char*, up<Code>*>>
                single_appearance_map{
                {KEY_VAL_TOP,    {"top",    &top}},
                {KEY_VAL_BOTTOM, {"bottom", &bottom}},
                {KEY_VAL_UNION,  {"union",  &union_}},
                {KEY_VAL_LEXER,  {"lexer",  &lexer_input}},
        };

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
                    register_action(
                            std::make_shared<CGAction>(iter, action_id(), iter->type == KEY_VAL_TOKEN_ASCII)
                    );
                    break;
                case KEY_VAL_TOKEN_TYPE:
                    register_action(std::make_shared<CGTypedAction>(iter, action_id()));
                    break;
                case KEY_VAL_TYPE:
                    register_grammar(std::make_shared<CGGrammarToken>(iter, 0));
                    break;
                case KEY_VAL_OPTION:
                    options.handle(iter);
                    break;
                case KEY_VAL_LEFT:
                case KEY_VAL_RIGHT:
                {
                    auto t = get_token(iter->key);
                    if (!t)
                    {
                        emit_error(&iter->position, "Undefined token");
                    }
                    else if (!dynamic_cast<CGAction*>(t.get()))
                    {
                        emit_error(&iter->position, "Cannot use %%type token in precedence");
                    }
                    else
                    {
                        if (precedence_mapping.find(t->id) != precedence_mapping.end())
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
            }
        }

        // All grammar tokens are initialized with their true ids
        for (auto &grammar_tok : grammar_tokens)
        {
            assert(!grammar_tok->id);
            grammar_tok->id = static_cast<int>(action_id() + grammar_i++);
        }

        // Add the augment token with the start type
        // TODO Clean this up
        const char ta[] = "TOK_AUGMENT";
        KeyVal v{.key = const_cast<char*>(start_type.c_str()), .value=const_cast<char*>(ta)};
        register_grammar(std::make_shared<CGGrammarToken>(&v, static_cast<int>(action_id() + grammar_i++)));

        action_tokens.emplace_back(new CGAction(nullptr, "TOK_EOF", 0));
        for (const auto &i : action_tokens)
        { tokens.push_back(i->name); }
        for (const auto &i : grammar_tokens)
        { tokens.push_back(i->name); }
    }

    void parse_lexer(const File* self)
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

            parse_builtin_lexer(self);
            lexer_type = LEXER_BUILTIN;
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
            lexer_type = LEXER_REFLEX_RAW;
        }
        else
        {
            // Manual lexer file input
            lexer_type = LEXER_REFLEX_FILE;
        }
    }

    void parse_builtin_lexer(const File* self)
    {
        int state_n = 0;
        ll_states.emplace_back(new CGLexerState(state_n++, "LEX_STATE_DEFAULT"));

        for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
        {
            if (iter->lexer_state)
            {
                // Make sure the name does not overlap
                bool duplicate_state = false;
                for (const auto &iter_state : ll_states)
                {
                    if (iter_state->get_name() == iter->lexer_state)
                    {
                        emit_error(&iter->position,
                                   "Multiple definition of lexer state '%s'",
                                   iter->lexer_state);
                        duplicate_state = true;
                        break;
                    }
                }

                if (duplicate_state)
                {
                    continue;
                }

                ll_states.emplace_back(new CGLexerState(state_n++, iter->lexer_state));
                auto &ls = ll_states[ll_states.size() - 1];

                for (struct LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
                {
                    assert(iter_s->regex);
                    assert(iter_s->function);
                    assert(!iter_s->lexer_state);
                    assert(!iter_s->state_rules);

                    ls->add_rule(m_engine, iter_s);
                }
            }
            else
            {
                // Add to the default lexing state
                ll_states[0]->add_rule(m_engine, iter);
            }
        }
    }

    void parse_grammar(const File* self)
    {
        if (start_type.empty() || start_token.empty())
        {
            emit_error(nullptr, "Undeclared %%start type");
            return;
        }

        // Add the augment rule
        static struct Token augment_rule_tok = {
                .position = NO_POSITION,
                .name = const_cast<char*>(start_token.c_str()),
                .next = nullptr,
        };

        static struct GrammarRuleSingleProto augment_rule_single_tok = {
                .position = NO_POSITION,
                .tokens = &augment_rule_tok,
                .function = nullptr,
                .next = nullptr,
        };

        // Add the augment rule
        grammar_n = 0;
        {
            std::vector<CGGrammar> augment_grammar;
            auto tok_augment_return = std::static_pointer_cast<CGGrammarToken>(get_token("TOK_AUGMENT"));
            if (!tok_augment_return)
            {
                emit_error(nullptr, "Expected augment rule to be a grammar token");
                return;
            }

            augment_grammar.emplace_back(this, tok_augment_return, &augment_rule_single_tok);
            gg_rules_cg.emplace("TOK_AUGMENT", std::move(augment_grammar));
        }

        for (struct GrammarRuleProto* rule_iter = self->grammar_rules;
             rule_iter;
             rule_iter = rule_iter->next)
        {
            sp<CGToken> tok = get_token(rule_iter->name);
            if (!tok)
            {
                emit_error(&rule_iter->position, "Could not find token for rule '%s'\n", rule_iter->name);
                continue;
            }
            else if (dynamic_cast<CGAction*>(tok.get()))
            {
                emit_error(&rule_iter->position, "Only %%type can be grammar rules");
                continue;
            }
            else if (gg_rules_cg.find(rule_iter->name) != gg_rules_cg.end())
            {
                emit_error(&rule_iter->position, "Redefinition of '%s' grammar", rule_iter->name);
                continue;
            }

            auto grammar = std::static_pointer_cast<CGGrammarToken>(tok);
            assert(grammar);

            std::vector<CGGrammar> grammars;
            for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
                 rule_single_iter;
                 rule_single_iter = rule_single_iter->next)
            {
                grammars.emplace_back(this, grammar, rule_single_iter);
                grammar_n++;
            }

            gg_rules_cg.emplace(rule_iter->name, grammars);
        }

        gg_rules.reserve(grammar_n);
        for (const auto &rules : gg_rules_cg)
        {
            for (const auto& rule : rules.second)
            {
                gg_rules.emplace_back(rule.initialize_grammar(this));
            }
        }
    }

    void write_header(std::ostream &os) const
    {
        std::string prefix_upper = options.prefix;
        std::transform(prefix_upper.begin(), prefix_upper.end(), prefix_upper.begin(), ::toupper);

        os << "#ifndef __NEOAST_" << prefix_upper << "_H__\n"
           << "#define __NEOAST_" << prefix_upper << "_H__\n\n"
           << "#ifdef __cplusplus\n"
           << "extern \"C\" {\n"
           << "#endif\n\n";

        os << "#ifdef __NEOAST_GET_TOKENS__\n";
        put_enum(NEOAST_ASCII_MAX + 1, tokens, os);
        os << "#endif // __NEOAST_GET_TOKENS__\n\n";

        os << "#ifdef __NEOAST_GET_STATES__\n";
        std::vector<std::string> state_names;
        for (const auto& i : ll_states) state_names.push_back(i->get_name());
        put_enum(0, state_names, os);
        os << "#endif // __NEOAST_GET_STATES__\n\n";

        // TODO Declare parsing/lexing functions

        os << "#ifdef __cplusplus\n"
           << "};\n"
           << "#endif // __cplusplus\n"
           << "#endif // __NEOAST_" << prefix_upper << "_H__\n";
    }

    void write_parser(std::ostream &os) const
    {
        os << "#define NEOAST_PARSER_CODEGEN___C\n"
           << "#define __NEOAST_GET_TOKENS__\n"
           << "#define __NEOAST_GET_STATES__\n"
           << "#include <neoast.h>\n"
           << "#include <string.h>\n";

        top->put(os);

        // Include necessary headers for chosen lexer
        switch (lexer_type)
        {
            case LEXER_BUILTIN:
                os << "#include <codegen/builtin_lexer/builtin_lexer.h>\n";
                break;
            case LEXER_REFLEX_RAW:
                os << "// TODO INCLUDE reflex lexer requirements\n";
                break;
            case LEXER_REFLEX_FILE:
                break;
        }

        // Act as a preprocessor and put the external header
        write_header(os);

        // Put the union definition
        os << "typedef union {";
        union_->put(os);
        os << "} " << CODEGEN_UNION << ";\n";

        // Place the wrapping structure definition
        if (options.lexer_opts & LEXER_OPT_TOKEN_POS)
        {
            os << "typedef struct {" CODEGEN_UNION " value; TokenPosition position;} " CODEGEN_STRUCT ";\n\n";
        }
        else
        {
            os << "typedef struct {" CODEGEN_UNION " value;}" CODEGEN_STRUCT ";\n\n";
        }

        // Put the destructors
        os << "// Destructors\n";
        for (const auto &iter : destructors)
        {
            os << "static void\n"
               << iter.first << "_destructor(" CODEGEN_UNION << "* self)\n{\n    ";
            iter.second->put(os, options, {iter.first}, "self", "", true);
            os << "\n}\n\n";
        }

        // Dump the destructor table
        os << "static const\n"
              "parser_destructor __neoast_token_destructors[] = {\n";
        for (const auto &token_name : tokens)
        {
            auto tok = get_token(token_name);

            if (!tok)
            {
                throw Exception("Failed to find token " + token_name);
            }
            else if (!dynamic_cast<CGTyped*>(tok.get()) ||
                     destructors.find(dynamic_cast<CGTyped*>(tok.get())->type)
                        == destructors.end())
            {
                os << variadic_string("        NULL, // %s\n", token_name.c_str());
            }
            else
            {
                os << variadic_string("        (parser_destructor) %s_destructor, // %s\n",
                                      dynamic_cast<CGTyped*>(tok.get())->type.c_str(),
                                      token_name.c_str());
            }
        }
        os << "};\n\n";

        write_lexer(os);

        // Put grammar actions
        int gg_i = 0;
        for (const auto &rules : gg_rules_cg)
        {
            for (const auto &rule : rules.second)
            { rule.put_action(os, options, gg_i++); }
        }

        // Put grammar table
        put_grammar_table(os);

        // Put token names
        os << "static const\n"
           << "char* __neoast_token_names[] = {\n";
        for (const auto &name : tokens)
        { os << "        \"" << name << "\",\n"; }
        os << "};";

        put_ascii_mappings(os);
        put_parser_definition(os);
        put_parsing_table(os);
        put_parsing_function(os);
        bottom->put(os);
    }

    void write_lexer(std::ostream &os) const
    {
        os << "// Lexer\n";
        switch (lexer_type)
        {
            case LEXER_BUILTIN:
            {
                for (const auto &ll_state : ll_states)
                {
                    ll_state->put(os);
                }

                put_lexer_states(os, ll_states);
            }
                break;
            case LEXER_REFLEX_RAW:
                break;
            case LEXER_REFLEX_FILE:
                break;
        }

    }

    void put_ascii_mappings(std::ostream &os) const
    {
        os << "static const\n"
           << "uint32_t __neoast_ascii_mappings[NEOAST_ASCII_MAX] = {\n";

        int i = 0;
        for (int row = 0; i < NEOAST_ASCII_MAX; row++)
        {
            os << "        ";
            for (int col = 0; col < 6 && i < NEOAST_ASCII_MAX; col++, i++)
            {
                os << variadic_string("0x%03X,%c", ascii_mappings[i], (col + 1 >= 6) ? '\n' : ' ');
            }
        }

        os << "};\n\n";
    }

    void put_grammar_table(std::ostream &os) const
    {
        // Parsing table

        // First put the grammar table
        os << "static const\n"
              "unsigned int grammar_token_table[] = {\n";

        uint32_t gg_i = 0;
        for (const auto &rules : gg_rules_cg)
        {
            os << variadic_string("        /* %s */\n", rules.first.c_str());
            for (const auto &rule : rules.second)
            {
                if (gg_i == 0)
                {
                    gg_i++;
                    os << "        /* ACCEPT */ ";
                }
                else
                {
                    os << variadic_string("        /* R%02d */ ", gg_i++);
                }

                rule.put_grammar_entry(os);
            }

            os << "\n";
        }
        os << "};\n\n";

        // Print the grammar rules
        uint32_t grammar_offset_i = 0;
        os << "static const\n"
              "GrammarRule __neoast_grammar_rules[] = {\n";

        gg_i = 1;
        for (const auto &rule : gg_rules)
        {
            if (rule.expr)
            {
                os << variadic_string(
                        "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d], .expr=(parser_expr) gg_rule_r%02d},\n",
                        tokens[rule.token - NEOAST_ASCII_MAX].c_str(),
                        rule.tok_n,
                        grammar_offset_i,
                        gg_i++);
            }
            else
            {
                os << variadic_string("        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d]},\n",
                                      tokens[rule.token - NEOAST_ASCII_MAX].c_str(),
                                      rule.tok_n,
                                      grammar_offset_i);
            }

            grammar_offset_i += rule.tok_n;
        }
        os << "};\n\n";
    }

    void put_parser_definition(std::ostream &os) const
    {
        os << variadic_string(
                "static GrammarParser parser = {\n"
                "        .grammar_n = %d,\n"
                "        .ascii_mappings = __neoast_ascii_mappings,\n"
                "        .grammar_rules = __neoast_grammar_rules,\n"
                "        .token_names = __neoast_token_names,\n"
                "        .destructors = __neoast_token_destructors,\n"
                "        .lexer_error = %s,\n"
                "        .parser_error = %s,\n"
                "        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,\n"
                "        .action_token_n = %d,\n"
                "        .lexer_opts = (lexer_option_t)%d,\n};\n\n",
                grammar_n,
                !options.lexing_error_cb.empty() ? options.lexing_error_cb.c_str() : "NULL",
                !options.syntax_error_cb.empty() ? options.syntax_error_cb.c_str() : "NULL",
                action_tokens.size(),
                options.lexer_opts
        );
    }

    void put_parsing_table(std::ostream &os) const
    {
        up<const char* []> token_names_ptr(new const char* [tokens.size()]);
        int i = 0;
        for (const auto &n : tokens)
        { token_names_ptr[i++] = n.c_str(); }

        GrammarParser parser {
                .ascii_mappings = ascii_mappings,
                .grammar_rules = gg_rules.data(),
                .token_names = token_names_ptr.get(),
                .grammar_n = grammar_n,
                .token_n = static_cast<uint32_t>(tokens.size()),
                .action_token_n = static_cast<uint32_t>(action_tokens.size()),
        };

        CanonicalCollection* cc = canonical_collection_init(&parser);
        canonical_collection_resolve(cc, options.parser_type);

        uint8_t error;
        up<uint8_t[]> precedence_table(new uint8_t[tokens.size()]);
        for (const auto& mapping : precedence_mapping)
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
        os << "static const\nuint32_t GEN_parsing_table[] = {\n";
        for (int state_i = 0; state_i < cc->state_n; state_i++)
        {
            os << "        ";
            for (int tok_i = 0; tok_i < cc->parser->token_n; tok_i++, i++)
            {
                os << variadic_string("0x%08X,%c", parsing_table[i], tok_i + 1 >= cc->parser->token_n ? '\n' : ' ');
            }
        }
        os << "};\n\n";

        canonical_collection_free(cc);
    }
    void put_table_debug(std::ostream& os,
                         const uint32_t* table,
                         const CanonicalCollection* cc) const
    {
        os << "// Token names:\n";

        std::string fallback;
        fallback.reserve(tokens.size() + 1);
        for (int i = 0; i < tokens.size() - 1; i++)
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
        if (!options.debug_ids.empty()) debug_ids = options.debug_ids.c_str();

        for (int i = 0; i < tokens.size() - 1; i++)
        {
            if (tokens[i].substr(0, 13) == "ASCII_CHAR_0x")
            {
                os << variadic_string("//  %s => '%c' ('%c')\n",
                                      tokens[i].c_str(), options.debug_ids[i],
                                      get_ascii_from_name(tokens[i].c_str()));
            }
            else
            {
                os << variadic_string("//  %s => '%c'\n", tokens[i].c_str(), options.debug_ids[i]);
            }
        }

        os << "\n";

        // This is a bit dirty since dump table takes a FILE*
        // and we are using an ostream not an ofstream.

        char *ptr = nullptr;
        size_t size = 0;
        FILE* fp = open_memstream(&ptr, &size);

        dump_table(table, cc, debug_ids, 0, fp, "//  ");
        os.write(ptr, static_cast<ssize_t>(size));
        os.flush();

        fclose(fp);
    }

    void put_parsing_function(std::ostream& os) const
    {
        // Dump the exported symbols
        os << "static int parser_initialized = 0;\n"
              "uint32_t "
           << options.prefix
           << "_init()\n{\n"
              "    if (!parser_initialized)\n"
              "    {\n"
              "        uint32_t error = parser_init(&parser);\n"
              "        if (error)\n"
              "            return error;\n\n"
              "        parser_initialized = 1;\n"
              "    }\n"
              "    return 0;\n"
              "}\n\n";

        os << "void "
           << options.prefix
           << "_free()\n"
              "{\n"
              "    if (parser_initialized)\n"
              "    {\n"
              "         parser_free(&parser);\n"
              "         parser_initialized = 0;\n"
              "    }\n"
              "}\n\n";

        if (options.lexer_opts & LEXER_OPT_TOKEN_POS)
        {
            os << variadic_string(
                    "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, "
                    "sizeof(" CODEGEN_STRUCT "), offsetof(" CODEGEN_STRUCT ", position));\n""}\n\n",
                    options.prefix.c_str(),
                    options.max_tokens,
                    options.parsing_stack_n);
        }
        else
        {
            os << variadic_string(
                    "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, "
                    "sizeof(" CODEGEN_STRUCT "), sizeof(" CODEGEN_STRUCT "));\n""}\n\n",
                    options.prefix.c_str(),
                    options.max_tokens,
                    options.parsing_stack_n);
        }

        os << "void "
           << options.prefix
           << "_free_buffers(void* self)\n"
              "{\n"
              "    parser_free_buffers((ParserBuffers*)self);\n"
              "}\n\n",

        os << variadic_string(
                "static " CODEGEN_UNION " t;\n"
                "typeof(t.%s) %s_parse(void* buffers, const char* input)\n"
                "{\n"
                "    parser_reset_buffers((ParserBuffers*)buffers);\n"
                "    \n"
                "    uint64_t input_len = strlen(input);\n"
                "    int32_t output_idx = parser_parse_lr(&parser, GEN_parsing_table,\n"
                "         (ParserBuffers*)buffers, input, input_len);\n"
                "    \n"
                "    if (output_idx < 0) return (typeof(t.%s))0;\n"
                "    return ((" CODEGEN_UNION "*)((ParserBuffers*)buffers)->value_table)[output_idx].%s;\n"
                "}\n"
                "typeof(t.%s) %s_parse_len(void* buffers, const char* input, uint64_t input_len)\n"
                "{\n"
                "    parser_reset_buffers((ParserBuffers*)buffers);\n"
                "    \n"
                "    int32_t output_idx = parser_parse_lr(&parser, GEN_parsing_table,\n"
                "         (ParserBuffers*)buffers, input, input_len);\n"
                "    \n"
                "    if (output_idx < 0) return (typeof(t.%s))0;\n"
                "    return ((" CODEGEN_UNION "*)((ParserBuffers*)buffers)->value_table)[output_idx].%s;\n"
                "}\n\n",
                start_type.c_str(), options.prefix.c_str(),
                start_type.c_str(), start_type.c_str(),
                start_type.c_str(), options.prefix.c_str(),
                start_type.c_str(), start_type.c_str());
    }

    int action_id() const
    { return static_cast<int>(action_tokens.size()); }

    sp<CGToken> get_token(const std::string &name) const
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

    void register_action(const sp<CGAction>& ptr)
    {
        if (get_token(ptr->name) != nullptr)
        {
            emit_error(ptr.get(), "Repeated definition of token");
            return;
        }

        if (ptr->is_ascii)
        {
            ascii_mappings[get_ascii_from_name(ptr->name.c_str())] = action_id() + NEOAST_ASCII_MAX;
        }

        action_tokens.push_back(ptr);
    }

    void register_grammar(const sp<CGGrammarToken>& ptr)
    {
        if (get_token(ptr->name) != nullptr)
        {
            emit_error(ptr.get(), "Repeated definition of token");
            return;
        }

        grammar_tokens.push_back(ptr);
    }

public:
    CodeGen(const File* self,
            std::ostream &cc_os,
            std::ostream &hh_os)
            : top(nullptr), bottom(nullptr), union_(nullptr),
              lexer_input(nullptr),
              options(), grammar_i(0), grammar_n(0),
              m_engine(), lexer_type(LEXER_BUILTIN)
    {
        parse_header(self);
        if (has_errors()) { return; }

        parse_lexer(self);
        if (has_errors()) { return; }

        parse_grammar(self);
        if (has_errors()) { return; }

        write_header(hh_os);
        if (has_errors()) { return; }

        write_parser(cc_os);
        if (has_errors()) { return; }
    }
};


CGGrammar::CGGrammar(CodeGen* cg, sp<CGGrammarToken> return_type_, const GrammarRuleSingleProto* self) :
        action(self->function ? self->function : "", &self->position), return_type(std::move(return_type_)), token_n(0),
        parent(self)
{
    assert(return_type.get());
    table_offset = cg->grammar_table.size();
    argument_types.push_back(return_type->type);

    for (const auto* iter = self->tokens; iter; iter = iter->next)
    {
        const auto t = cg->get_token(iter->name);
        if (!t)
        {
            emit_error(&iter->position, "Undeclared token '%s'", iter->name);
            continue;
        }

        cg->grammar_table.push_back(t->id + NEOAST_ASCII_MAX);
        token_n++;

        const auto* t_casted = dynamic_cast<CGTyped*>(t.get());
        if (t_casted)
        {
            argument_types.push_back(t_casted->type);
        }
        else
        {
            argument_types.emplace_back("");
        }
    }
}

GrammarRule CGGrammar::initialize_grammar(CodeGen* cg) const
{
    GrammarRule gg;
    gg.expr = action.empty() ? nullptr : reinterpret_cast<parser_expr>(0x2);
    gg.token = return_type->id + NEOAST_ASCII_MAX;
    gg.grammar = reinterpret_cast<const uint32_t*>(&cg->grammar_table[table_offset]);
    gg.tok_n = token_n;
    return gg;
}


int codegen_write(const char* grammar_file_path,
                  const File* self,
                  const char* output_file_cc,
                  const char* output_file_hh)
{
    if (grammar_file_path)
    {
        grammar_filename = grammar_file_path;
    }

    std::ofstream os(output_file_cc);
    std::ofstream hs(output_file_hh);

    try
    {
        CodeGen c(self, os, hs);
    }
    catch (const ASTException& e)
    {
        emit_error(e.position(), e.what());
    }
    catch (const Exception& e)
    {
        emit_error(nullptr, e.what());
    }
    catch (const std::exception& e)
    {
        emit_error(nullptr, "System exception: %s", e.what());
    }

    os.close();
    hs.close();
    return has_errors();
}
