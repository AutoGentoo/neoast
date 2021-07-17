//
// Created by tumbar on 6/20/21.
//

#ifndef NEOAST_CODEGEN_PRIV_H
#define NEOAST_CODEGEN_PRIV_H


#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <reflex.h>
#include <reflex/matcher.h>
#include "cg_util.h"
#include "regex.h"


#define CODEGEN_STRUCT "NeoastValue"
#define CODEGEN_UNION "NeoastUnion"


struct CGToken;
struct CGGrammarToken;
struct CGTyped;
struct CGAction;
struct CodeGen;


extern const TokenPosition NO_POSITION;


extern std::string grammar_filename;


template<typename T>
using up = std::unique_ptr<T>;

template<typename T>
using sp = std::shared_ptr<T>;

struct Code : public TokenPosition
{
    std::string code;   ///< line of code
    std::string file;   ///< source filename

    Code(const char* value, const TokenPosition* position) :
            code(value), file(grammar_filename), TokenPosition(*position) {}

    explicit Code(const KeyVal* self)
            : code(self->value),
              file(grammar_filename),
              TokenPosition(self->position)
    {
    }

    bool empty() const { return code.empty(); }

    void put(std::ostream& ss) const
    {
        if (line && !file.empty())
        {
            ss << "#line "
               << line
               << " \"" << file.c_str() << "\"\n";
        }
        ss << code;
    }

    void put(std::ostream& os,
             const Options& options,
             const std::vector<std::string>& argument_replace,
             const std::string& zero_arg,
             const std::string& non_zero_arg,
             bool is_union = false) const
    {
        if (line && !file.empty())
        {
            os << "#line "
               << line
               << " \"" << file.c_str() << "\"\n";
        }

        auto redzones = mark_redzones(code);

        reflex::Pattern argument_pattern("[$](?:[$]|p?[0-9]+)");
        reflex::Matcher m(argument_pattern, code);

        size_t last_position = 0;
        for (auto& match : m.find)
        {
            if (m.first() > last_position)
            {
                os << code.substr(last_position, m.first() - last_position);
            }

            last_position = m.last();
            if (in_redzone(redzones, match))
            {
                os << m.text();
                continue;
            }

            TokenPosition match_pos = {line, static_cast<uint32_t>(col_start + match.first())};
            if (match.text()[1] == '$')
            {
                if (is_union)
                {
                    os << zero_arg << "->" << argument_replace[0];
                }
                else
                {
                    os << zero_arg << "->value." << argument_replace[0];
                }
            }
            else if (match.text()[1] == 'p')
            {
                if (is_union)
                {
                    emit_error(&match_pos, "Illegal yyval position $p in lexer rule");
                    continue;
                }

                size_t idx;
                idx = std::stol(match.text() + 2, nullptr, 10);

                if (!(options.lexer_opts & LEXER_OPT_TOKEN_POS))
                {
                    emit_error(&match_pos, "Attempting to get token position without track_position=\"TRUE\"");
                    break;
                }
                else if (idx >= argument_replace.size())
                {
                    emit_error(&match_pos, "Position index out of range, function only has '%d' arguments",
                               argument_replace.size());
                    continue;
                }
                else if (idx == 0)
                {
                    emit_error(&match_pos, "Invalid position argument index '0' use $p1");
                    continue;
                }

                if (!options.track_position_type.empty())
                {
                    os << variadic_string("((const %s*)&%s[%d].position)",
                                          options.track_position_type.c_str(),
                                          non_zero_arg.c_str(),
                                          idx - 1);
                }
                else
                {
                    os << variadic_string("((const TokenPosition*)&args[%d].position)", idx - 1);
                }
            }
            else
            {
                if (is_union)
                {
                    emit_error(&match_pos, "Illegal non destination $N in lexer rule");
                    continue;
                }

                size_t idx;
                try
                {
                    idx = std::stol(match.text() + 1, nullptr, 10);
                }
                catch (const std::exception& e)
                {
                    emit_error(&match_pos, "Invalid argument, excepted integer: %s", e.what());
                    continue;
                }

                if (idx >= argument_replace.size())
                {
                    emit_error(&match_pos, "Argument index out of range, function only has '%d' arguments",
                               argument_replace.size());
                    continue;
                }
                else if (idx == 0)
                {
                    emit_error(&match_pos, "Invalid argument index '0', use '$$' for destination");
                    continue;
                }
                else if (argument_replace[idx].empty())
                {
                    emit_error(&match_pos, "Argument '%d' does not have type", idx);
                    continue;
                }

                os << non_zero_arg << "[" << idx - 1 << "].value." << argument_replace[idx];
            }
        }

        os << code.substr(last_position) << "\n";
    }

private:
    static bool in_redzone(const std::vector<std::pair<int, int>>& redzones, const reflex::AbstractMatcher& m)
    {
        for (const auto& iter : redzones)
        {
            if (iter.first > m.first() && iter.second < m.last())
            {
                return true;
            }

            // Future redzones are always outside the bounds of this match
            if (iter.first > m.last())
            {
                return false;
            }
        }

        return false;
    }
};

struct CGToken : public TokenPosition
{
    std::string name;
    int id;

    CGToken(const TokenPosition* position,
            std::string name,
            int id)
            : TokenPosition(position ? *position : NO_POSITION),
            name(std::move(name)), id(id) {}

    // Make all children virtual so that we have access to dynamic casting
    virtual ~CGToken() = default;
};

struct CGTyped : public CGToken
{
    std::string type;           //!< Field in union

    CGTyped(const KeyVal* self, int id)
            : CGToken(&self->position, self->value, id), type(self->key) {}
};

struct CGGrammarToken : public CGTyped
{
    CGGrammarToken(const KeyVal* self, int id) : CGTyped(self, id) {}
};

struct CGAction : public CGToken
{
    bool is_ascii;

    CGAction(const KeyVal* self, int id, bool is_ascii = false)
            : CGToken(&self->position, self->value, id), is_ascii(is_ascii) {}

    CGAction(const TokenPosition* position, std::string name,
             int id, bool is_ascii = false)
            : CGToken(position, std::move(name), id), is_ascii(is_ascii) {}
};

struct CGTypedAction : public CGTyped, public CGAction
{
    CGTypedAction(const KeyVal* self, int id) : CGTyped(self, id), CGAction(self, id) {}
};


struct CGGrammar
{
    const GrammarRuleSingleProto* parent;
    sp<CGGrammarToken> return_type;
    std::vector<std::string> argument_types;

    Code action;

    uint32_t table_offset;
    uint32_t token_n;

    CGGrammar(CodeGen* cg, sp<CGGrammarToken> return_type, const GrammarRuleSingleProto* self);

    GrammarRule initialize_grammar(CodeGen* cg) const;

    void put_action(std::ostream& os, const Options& options, int i) const
    {
        if (action.empty())
        {
            return;
        }

        os << "static void\n"
           << variadic_string("gg_rule_r%02d(%s* dest, %s* args)\n{\n",
                              i, CODEGEN_STRUCT, CODEGEN_STRUCT)
           << "    (void) dest;\n"
           << "    (void) args;\n";

        action.put(os, options, argument_types, "dest", "args");

        os << "}\n\n";
    }

    void put_grammar_entry(std::ostream& os) const
    {
        for (struct Token* tok = parent->tokens; tok; tok = tok->next)
        {
            os << variadic_string("%s,%c", tok->name, tok->next ? ' ' : '\n');
        }

        if (!parent->tokens)
        {
            os << "// empty rule\n";
        }
    }
};

#endif //NEOAST_CODEGEN_PRIV_H
