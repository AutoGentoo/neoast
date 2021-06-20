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


#include <reflex.h>
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



static inline
void put_grammar_table_and_rules(
        struct GrammarRuleProto* rule_symbols,
        GrammarRule* rules,
        const char* const* tokens,
        uint32_t rule_n,
        FILE* fp)
{
    // First put the grammar table
    fputs("static const\nunsigned int grammar_token_table[] = {\n", fp);
    uint32_t grammar_i = 0;
    for (struct GrammarRuleProto* rule_iter = rule_symbols;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        fprintf(fp, "        /* %s */\n", rule_iter->name);
        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            if (grammar_i == 0)
            {
                grammar_i++;
                fputs("        /* ACCEPT */ ", fp);
            }
            else
            {
                fprintf(fp,"        /* R%02d */ ", grammar_i++);
            }
            if (rule_single_iter->tokens)
            {
                for (struct Token* tok = rule_single_iter->tokens; tok; tok = tok->next)
                {
                    fprintf(fp, "%s,%c", tok->name, tok->next ? ' ' : '\n');
                }
            } else
            {
                fputs("// empty rule\n", fp);
            }
        }

        fputc('\n', fp);
    }
    fputs("};\n\n", fp);

    // Print the grammar rules
    uint32_t grammar_offset_i = 0;
    fputs("static const\nGrammarRule __neoast_grammar_rules[] = {\n", fp);
    for (int i = 0; i < rule_n; i++)
    {
        if (rules[i].expr)
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d], .expr=(parser_expr) gg_rule_r%02d},\n",
                    tokens[rules[i].token - NEOAST_ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i,
                    i);
        }
        else
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d]},\n",
                    tokens[rules[i].token - NEOAST_ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i);
        }

        grammar_offset_i += rules[i].tok_n;
    }
    fputs("};\n\n", fp);
}

static inline
void put_parsing_table(const uint32_t* parsing_table, CanonicalCollection* cc, FILE* fp)
{
    int i = 0;
    fputs("static const\nuint32_t GEN_parsing_table[] = {\n", fp);
    for (int state_i = 0; state_i < cc->state_n; state_i++)
    {
        fputs("        ", fp);
        for (int tok_i = 0; tok_i < cc->parser->token_n; tok_i++, i++)
        {
            fprintf(fp,"0x%08X,%c", parsing_table[i], tok_i + 1 >= cc->parser->token_n ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
}

static inline
void put_ascii_mappings(const uint32_t ascii_mappings[NEOAST_ASCII_MAX], FILE* fp)
{
    fputs("static const\nuint32_t __neoast_ascii_mappings[NEOAST_ASCII_MAX] = {\n", fp);
    int i = 0;
    for (int row = 0; i < NEOAST_ASCII_MAX; row++)
    {
        fputs("        ", fp);
        for (int col = 0; col < 6 && i < NEOAST_ASCII_MAX; col++, i++)
        {
            fprintf(fp, "0x%03X,%c", ascii_mappings[i], (col + 1 >= 6) ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
}


std::string grammar_filename;


struct CodeGen
{
    MacroEngine m_engine;

    up<Code> top;
    up<Code> bottom;
    up<Code> union_;

    std::string start_type;
    up<Code> start;

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

    std::vector<CGLexerState> ll_states;
    std::vector<CGGrammar> gg_rules_cg;
    up<GrammarRule[]> gg_rules;
    std::vector<int32_t> grammar_table;

    Options options;

    int grammar_i;
    uint32_t ascii_mappings[NEOAST_ASCII_MAX] = {0};

    CodeGen(const File* self,
            std::ostream& cc_os,
            std::ostream& hh_os)
            : top(nullptr), bottom(nullptr), union_(nullptr), start(nullptr),
              gg_rules(nullptr), lexer_input(nullptr),
              options(), grammar_i(0), m_engine(), lexer_type(LEXER_BUILTIN)
    {
        parse_header(self);
        if (has_errors()) { return; }

        parse_lexer(self);
        if (has_errors()) { return; }

        parse_grammar(self);
        if (has_errors()) { return; }

        write_header(hh_os);
        if (has_errors()) { return; }
    }

    void parse_header(const File* self)
    {
        // Setup all tokens and grammar rules as well as
        // other header information

        std::map<key_val_t, std::pair<const char*, up<Code>*>>
                single_appearance_map{
                {KEY_VAL_TOP, {"top", &top}},
                {KEY_VAL_BOTTOM, {"bottom", &bottom}},
                {KEY_VAL_UNION, {"union", &union_}},
                {KEY_VAL_START, {"start", &start}},
                {KEY_VAL_LEXER, {"lexer", &lexer_input}},
        };

        for (KeyVal* iter = self->header; iter; iter = iter->next)
        {
            switch (iter->type)
            {
                case KEY_VAL_TOP:
                case KEY_VAL_BOTTOM:
                case KEY_VAL_UNION:
                case KEY_VAL_START:
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
                    register_action(new CGAction(iter, action_id(), iter->type == KEY_VAL_TOKEN_ASCII));
                    break;
                case KEY_VAL_TOKEN_TYPE:
                    register_action(new CGTypedAction(iter, action_id()));
                    break;
                case KEY_VAL_TYPE:
                    register_grammar(new CGGrammarToken(iter, 0));
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
                    else if (!t->is_typed())
                    {
                        emit_error(&iter->position, "Token with no type cannot have a destructor");
                    }
                    else
                    {
                        // TODO Setup precedence table
                    }
                }
                    break;
                case KEY_VAL_MACRO:
                    m_engine.add(iter->key, iter->value);
                    break;
                case KEY_VAL_DESTRUCTOR:
                {
                    auto t = get_token(iter->key);
                    if (!t)
                    {
                        emit_error(&iter->position, "Undefined token");
                    }
                    else if (!t->is_typed())
                    {
                        emit_error(&iter->position, "Token with no type cannot have a destructor");
                    }
                    else
                    {
                        const auto& type = std::static_pointer_cast<CGTyped>(t)->type;
                        if (destructors.find(type) != destructors.end())
                        {
                            emit_error(&iter->position, "Destructor for type '%s' is already defined",
                                       type.c_str());
                        }

                        destructors[type] = std::make_unique<Code>(iter);
                    }
                }
                    break;
            }
        }

        // All grammar tokens are initialized with their true ids
        for (auto& grammar_tok : grammar_tokens)
        {
            assert(!grammar_tok->id);
            grammar_tok->id = action_id() + grammar_i++;
        }

        // Add the augment token with the start type
        // TODO Clean this up
        const char ta[] = "TOK_AUGMENT";
        KeyVal v {.key = const_cast<char*>(start_type.c_str()), .value=const_cast<char*>(ta)};
        auto* augment = new CGGrammarToken(&v, action_id() + grammar_i++);
        register_grammar(augment);

        tokens.emplace_back("TOK_EOF");
        for (const auto& i : action_tokens) { tokens.push_back(i->name); }
        for (const auto& i : grammar_tokens) { tokens.push_back(i->name); }
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
        ll_states.emplace_back(state_n++, "LEX_STATE_DEFAULT");

        for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
        {
            if (iter->lexer_state)
            {
                // Make sure the name does not overlap
                bool duplicate_state = false;
                for (const auto& iter_state : ll_states)
                {
                    if (iter_state.get_name() == iter->lexer_state)
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

                ll_states.emplace_back(state_n++, iter->lexer_state);
                auto& ls = ll_states[ll_states.size() - 1];

                for (struct LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
                {
                    assert(iter_s->regex);
                    assert(iter_s->function);
                    assert(!iter_s->lexer_state);
                    assert(!iter_s->state_rules);

                    ls.add_rule(m_engine, iter_s);
                }
            }
            else
            {
                // Add to the default lexing state
                ll_states[0].add_rule(m_engine, iter);
            }
        }
    }
    void parse_grammar(const File* self)
    {
        // Add the augment rule
        struct Token augment_rule_tok = {
                .name = const_cast<char*>(start_type.c_str()),
                .next = nullptr,
        };

        struct GrammarRuleSingleProto augment_rule = {
                .tokens = &augment_rule_tok,
                .next = nullptr,
        };

        struct GrammarRuleProto augment_rule_parent = {
                .name = (char*)"TOK_AUGMENT",
                .rules = &augment_rule,
                .next = self->grammar_rules,
        };

        struct GrammarRuleProto* all_rules = &augment_rule_parent;

        for (struct GrammarRuleProto* rule_iter = all_rules;
             rule_iter;
             rule_iter = rule_iter->next)
        {
            sp<CGToken> tok = get_token(rule_iter->name);
            if (!tok)
            {
                emit_error(&rule_iter->position, "Could not find token for rule '%s'\n", rule_iter->name);
                continue;
            }
            else if (tok->is_action())
            {
                emit_error(&rule_iter->position, "Only %%type can be grammar rules");
                continue;
            }

            auto grammar = std::static_pointer_cast<CGGrammarToken>(tok);
            assert(grammar);

            for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
                 rule_single_iter;
                 rule_single_iter = rule_single_iter->next)
            {
                gg_rules_cg.emplace_back(this, grammar, rule_single_iter);
            }
        }

        gg_rules = up<GrammarRule[]>(new GrammarRule[gg_rules_cg.size()]);
        int i = 0;
        for (const auto& rule : gg_rules_cg)
        {
            gg_rules[i] = rule.initialize_grammar(this);
        }
    }

    void write_header(std::ostream& os) const
    {
        std::string prefix_upper = options.prefix;
        std::transform(prefix_upper.begin(), prefix_upper.end(),prefix_upper.begin(), ::toupper);

        os << "#ifndef __NEOAST_" << prefix_upper << "_H__\n"
           << "#define __NEOAST_" << prefix_upper << "_H__\n\n"
           << "#ifdef __cplusplus\n"
           << "extern \"C\" {\n"
           << "#endif\n\n";

        os << "#ifdef __NEOAST_GET_TOKENS__\n";

        put_enum(NEOAST_ASCII_MAX + 1, tokens, os);

        os << "}\n#endif\n";

        // TODO Declare parsing/lexing functions

        os << "#ifdef __cplusplus\n"
           << "};\n"
           << "#endif\n\n"
           << "#endif\n";
    }
    void write_parser(std::ostream& os) const
    {
        os << "#define NEOAST_PARSER_CODEGEN___C\n"
           << "#define __NEOAST_GET_TOKENS__\n"
           << "#include <neoast.h>\n"
           << "#include <string.h>\n";

        top->put(os);

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
        for (const auto& iter : destructors)
        {
            os << "static void\n"
               << iter.first << "_destructor(" CODEGEN_UNION << "* self)\n{\n    ";
            iter.second->put(os, options, {iter.first}, "self", "");
            os << "\n}\n\n";
        }

        // Dump the destructor table
        os << "static const\n"
              "parser_destructor __neoast_token_destructors[] = {\n";
        for (const auto& token_name : tokens)
        {
            auto tok = get_token(token_name);

            if (!tok->is_typed() || destructors.find(std::static_pointer_cast<CGTyped>(tok)->type) == destructors.end())
            {
                os << variadic_string("        NULL, // %s", token_name.c_str());
            }
            else
            {
                os << variadic_string("        (parser_destructor) %s_destructor, // %s",
                                      std::static_pointer_cast<CGTyped>(tok)->type.c_str(),
                                      token_name.c_str());
            }
        }
        os << "};\n\n";

        write_lexer(os);

        int gg_i = 0;
        for (const auto& rule : gg_rules_cg)
        {
            rule.put(os, options, gg_i++);
        }
    }
    void write_lexer(std::ostream& os) const
    {
        os << "// Lexer\n";
        switch (lexer_type)
        {
            case LEXER_BUILTIN:
            {
                for (const auto& ll_state : ll_states)
                {
                    ll_state.put(os);
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

    int action_id() const { return static_cast<int>(action_tokens.size()); }

    sp<CGToken> get_token(const std::string& name) const
    {
        for (const auto& i : action_tokens)
        {
            if (i->name == name)
            {
                return i;
            }
        }

        for (const auto& i : grammar_tokens)
        {
            if (i->name == name)
            {
                return i;
            }
        }

        return nullptr;
    }

    void register_action(CGAction* ptr)
    {
        if (get_token(ptr->name) != nullptr)
        {
            emit_error(ptr, "Repeated definition of token");
            return;
        }

        if (ptr->is_ascii)
        {
            ascii_mappings[get_ascii_from_name(ptr->name.c_str())] = action_id() + NEOAST_ASCII_MAX;
        }

        action_tokens.emplace_back(ptr);
    }

    void register_grammar(CGGrammarToken* ptr)
    {
        if (get_token(ptr->name) != nullptr)
        {
            emit_error(ptr, "Repeated definition of token");
            return;
        }

        grammar_tokens.emplace_back(ptr);
    }
};


CGGrammar::CGGrammar(CodeGen* cg, sp<CGGrammarToken> return_type, const GrammarRuleSingleProto* self) :
        action(self->function, &self->position), return_type(std::move(return_type)), token_n(0)
{
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

        if (t->is_typed())
        {
            argument_types.push_back(std::static_pointer_cast<CGTyped>(t)->type);
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
    gg.expr = nullptr;
    gg.token = return_type->id + NEOAST_ASCII_MAX;
    gg.grammar = reinterpret_cast<const uint32_t*>(&cg->grammar_table[table_offset]);
    gg.tok_n = token_n;
    return gg;
}


int codegen_write(const char* grammar_file_path,
                  const File* self,
                  FILE* fp)
{
    // Codegen steps:
    //   1. Write the header
    //   2. Generate enums and union
    //   3. Dump lexer actions
    //   4. Dump parser actions
    //   5. Dump lexer rules structures (all states)
    //   6. Dump parser rules structure
    //   7. Generate table and write
    //   8. Generate entry point and buffer creation


    put_ascii_mappings(ascii_mappings, fp);

    fprintf(fp, "static GrammarParser parser = {\n"
                "        .grammar_n = %d,\n"
                "        .lex_state_n = %d,\n"
                "        .lex_n = lexer_rule_n,\n"
                "        .ascii_mappings = __neoast_ascii_mappings,\n"
                "        .lexer_rules = __neoast_lexer_rules,\n"
                "        .grammar_rules = __neoast_grammar_rules,\n"
                "        .token_names = __neoast_token_names,\n"
                "        .destructors = __neoast_token_destructors,\n"
                "        .lexer_error = %s,\n"
                "        .parser_error = %s,\n"
                "        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,\n"
                "        .action_token_n = %d,\n"
                "        .lexer_opts = (lexer_option_t)%d,\n",
            grammar_n, lex_state_n,
            options.lexing_error_cb ? options.lexing_error_cb : "NULL",
            options.syntax_error_cb ? options.syntax_error_cb : "NULL",
            action_n, options.lexer_opts
    );
    fputs("};\n\n", fp);

    EXIT_IF_ERRORS;

    CanonicalCollection* cc = canonical_collection_init(&parser);
    canonical_collection_resolve(cc, options.parser_type);
    DESTROY_ME(cc, (void (*) (void*))canonical_collection_free);

    uint8_t error;
    uint32_t* parsing_table = canonical_collection_generate(cc, precedence_table, &error);
    DESTROY_ME(parsing_table, free);

    if (options.debug_table)
    {
        fprintf(fp, "// Token names:\n");
        char* fallback = malloc(token_n + 1);
        for (int i = 0; i < token_n - 1; i++)
        {
            if (i == 0)
            {
                fallback[i] = '$';
            }
            else if (i < action_n)
            {
                fallback[i] = (char)('a' + (char)i - 1);
            }
            else
            {
                fallback[i] = (char)('A' + (char)i - (action_n));
            }
        }

        if (!options.debug_ids) options.debug_ids = fallback;
        for (int i = 0; i < token_n - 1; i++)
        {
            if (strncmp(tokens[i], "ASCII_CHAR_0x", 13) == 0)
            {
                fprintf(fp, "//  %s => '%c' ('%c')\n",
                        tokens[i], options.debug_ids[i],
                        get_ascii_from_name(tokens[i]));
            }
            else
            {
                fprintf(fp, "//  %s => '%c'\n", tokens[i], options.debug_ids[i]);
            }
        }

        fputs("\n", fp);
        dump_table(parsing_table, cc, options.debug_ids, 0, fp, "//  ");
        free(fallback);
    }

    // Dump the actual parsing table
    put_parsing_table(parsing_table, cc, fp);

    // Dump the exported symbols
    fprintf(fp, "static int parser_initialized = 0;\n"
                "uint32_t %s_init()\n{\n"
                "    if (!parser_initialized)\n"
                "    {\n"
                "        uint32_t error = parser_init(&parser);\n"
                "        if (error)\n"
                "            return error;\n\n"
                "        parser_initialized = 1;\n"
                "    }\n"
                "    return 0;\n"
                "}\n\n",
                options.prefix);
    fprintf(fp, "void %s_free()\n"
                "{\n"
                "    if (parser_initialized)\n"
                "    {\n"
                "         parser_free(&parser);\n"
                "         parser_initialized = 0;\n"
                "    }\n"
                "}\n\n",
                options.prefix);
    if (options.lexer_opts & LEXER_OPT_TOKEN_POS)
    {
        fprintf(fp, "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, %lu, %lu, "
                    "sizeof(" CODEGEN_STRUCT "), offsetof(" CODEGEN_STRUCT ", position));\n"
                    "}\n\n",
                options.prefix,
                options.max_lex_tokens,
                options.max_token_len,
                options.max_lex_state_depth,
                options.parsing_stack_n);
    }
    else
    {
        fprintf(fp, "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, %lu, %lu, "
                    "sizeof(" CODEGEN_STRUCT "), sizeof(" CODEGEN_STRUCT "));\n"
                    "}\n\n",
                options.prefix,
                options.max_lex_tokens,
                options.max_token_len,
                options.max_lex_state_depth,
                options.parsing_stack_n);
    }

    fprintf(fp, "void %s_free_buffers(void* self)\n"
                "{\n"
                "    parser_free_buffers((ParserBuffers*)self);\n"
                "}\n\n",
                options.prefix);

    fprintf(fp, "static " CODEGEN_UNION " t;\n"
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
                "}\n"
                "\n"
                "#endif\n",
            _start->key, options.prefix,
            _start->key, _start->key,
            _start->key, options.prefix,
            _start->key, _start->key);

    if (_bottom)
    {
        fputs(_bottom->value, fp);
        fputc('\n', fp);
    }

    EXIT_IF_ERRORS;
    DESTROY_ALL;
    return error;
}
