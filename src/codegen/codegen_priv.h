//
// Created by tumbar on 6/20/21.
//

#ifndef NEOAST_CODEGEN_PRIV_H
#define NEOAST_CODEGEN_PRIV_H


#include <memory>
#include <string>
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

    void put(std::ostream& ss) const
    {
        if (line && !file.empty())
        {
            ss << "#line "
               << line
               << " \"" << file.c_str() << "\"\n";
        }
        ss << code << "\n";
    }

    void put(std::ostream& os,
             const Options& options,
             const std::vector<std::string>& argument_replace,
             const std::string& zero_arg,
             const std::string& non_zero_arg,
             const std::string& position_field = "position") const
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
                os << code.substr(last_position, m.first());
            }

            last_position = m.last();
            if (in_redzone(redzones, match))
            {
                os << m.text();
                continue;
            }

            if (match.text()[1] == '$')
            {
                os << zero_arg << "->" << argument_replace[0];
            }
            else if (match.text()[1] == 'p')
            {
                size_t idx;
                idx = std::stol(match.text() + 2, nullptr, 10);

                TokenPosition match_pos = {line, static_cast<uint32_t>(col_start + match.first())};
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
                    os << variadic_string("((const %s*)&args[%d].position)",
                                          options.track_position_type.c_str(), idx - 1);
                }
                else
                {
                    os << variadic_string("((const TokenPosition*)&args[%d].position)", idx - 1);
                }
            }
            else
            {
                size_t idx;
                idx = std::stol(match.text() + 2, nullptr, 10);

                TokenPosition match_pos = {line, static_cast<uint32_t>(col_start + match.first())};
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

                os << non_zero_arg << "[" << idx - 1 << "]." << argument_replace[idx];
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
            : TokenPosition(*position), name(std::move(name)), id(id) {}

    virtual bool is_action() const { return false; };
    virtual bool is_typed() const { return false; };
};


struct CGTyped : public CGToken
{
    std::string type;           //!< Field in union

    CGTyped(const KeyVal* self, int id)
            : CGToken(&self->position, self->value, id), type(self->key) {}

    bool is_typed() const override { return true; }
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

    // Make this a virtual class so we that we have access
    // to dynamic casting
    bool is_action() const override { return true; }
};

struct CGTypedAction : public CGTyped, public CGAction
{
    CGTypedAction(const KeyVal* self, int id) : CGTyped(self, id), CGAction(self, id) {}
};


struct CGGrammar
{

    sp<CGGrammarToken> return_type;
    std::vector<std::string> argument_types;

    Code action;

    uint32_t table_offset;
    uint32_t token_n;

    CGGrammar(CodeGen* cg, sp<CGGrammarToken> return_type, const GrammarRuleSingleProto* self);

    GrammarRule initialize_grammar(CodeGen* cg) const;

    void put(std::ostream& os, const Options& options, int i) const
    {
        os << "static void\n"
           << variadic_string("gg_rule_r%02d(%s* dest, %s* args)\n{\n",
                              i, CODEGEN_UNION, CODEGEN_UNION)
           << "(void) dest;"
           << "(void) args;";

        action.put(os, options, argument_types, "dest", "args");
    }
};

#endif //NEOAST_CODEGEN_PRIV_H
