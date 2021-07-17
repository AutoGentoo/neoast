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

#include <vector>
#include "builtin_lexer.h"
#include <reflex/pattern.h>
#include <reflex/matcher.h>
#include <reflex/abslexer.h>
#include <sstream>


struct BuiltinLexerState
{
    std::vector<const LexerRule*> rules;
    std::string regex_str;
    reflex::Pattern pattern;

    BuiltinLexerState(const LexerRule* rules_,
                      const uint32_t rules_n)
    {
        rules.reserve(rules_n);

        for (uint32_t i = 0; i < rules_n; i++)
        {
            rules.push_back(&rules_[i]);
        }

        // Build the pattern using the given rules using the following
        // construction rules:
        // (?mx)((?:<rule1>))|((?:<rule2>))
        std::stringstream os;
        os << "(?mx)";
        std::string sep;
        for (const auto* rule : rules)
        {
            assert(rule->regex);
            os << sep << "((?:"
               << rule->regex
               << "))";
            sep = "|";
        }

        regex_str = os.str();
        pattern = reflex::Pattern(regex_str);
    }
};


struct BuiltinLexer
{
    std::vector<BuiltinLexerState> states;
    ll_error_cb error_cb;

    BuiltinLexer(const LexerRule* const* rules,
                 const uint32_t rules_n[],
                 uint32_t state_n,
                 ll_error_cb error) : error_cb(error)
    {
        assert(state_n);
        states.reserve(state_n);

        for (int i = 0; i < state_n; i++)
        {
            states.emplace_back(rules[i], rules_n[i]);
        }
    }
};


class BuiltinLexerSession : public reflex::AbstractLexer<reflex::Matcher>
{
    const BuiltinLexer* parent;
    const reflex::Input &input;
    ParsingStack* lexer_states;

public:
    BuiltinLexerSession(const reflex::Input &input, const BuiltinLexer *parent,
                        std::ostream &os = std::cout) :
            AbstractLexer(input, os), parent(parent), input(input)
    {

        lexer_states = parser_allocate_stack(32);
        NEOAST_STACK_PUSH(lexer_states, 0);

        if (!has_matcher())
        {
            matcher(new Matcher(parent->states[0].pattern, stdinit(), this));
        }
    }

    int next(void* ll_val)
    {
        int tok = -1;
        while (tok < 0)
        {
            int current_state = NEOAST_STACK_PEEK(lexer_states);
            matcher().pattern(parent->states[current_state].pattern);
            const BuiltinLexerState& state = parent->states[current_state];

            auto rule_out = matcher().scan();
            if (rule_out == 0 && matcher().at_end() )
            {
                // EOF
                return 0;
            }

            int rule_idx = static_cast<int>(rule_out) - 1;
            if (rule_idx >= state.rules.size())
            {
                // Invalid token
                TokenPosition p {
                        .line = static_cast<uint32_t>(lineno()),
                        .col_start=static_cast<uint32_t>(columno())};

                if (parent->error_cb)
                {
                    parent->error_cb(input.cstring(), &p);
                }
                else
                {
                    std::cerr << "Failed to match token near on line:col "
                              << p.line << ":" << p.col_start << " (state " << current_state << ")\n";
                }

                return -1;
            }
            else
            {
                // Run token action
                if (state.rules[rule_idx]->tok)
                {
                    tok = state.rules[rule_idx]->tok;
                }
                else if (state.rules[rule_idx]->expr)
                {
                    TokenPosition p {
                        .line = static_cast<uint32_t>(lineno()),
                        .col_start=static_cast<uint32_t>(columno())};

                    tok = state.rules[rule_idx]->expr(text(), ll_val, size(), lexer_states, &p);
                }
            }
        }

        return tok;
    }

    ~BuiltinLexerSession() override
    {
        parser_free_stack(lexer_states);
        lexer_states = nullptr;
    }
};


void* builtin_lexer_new(
        const LexerRule* const rules[],
        const uint32_t rules_n[],
        uint32_t state_n,
        ll_error_cb error_cb)
{
    return new BuiltinLexer(rules, rules_n, state_n, error_cb);
}

void builtin_lexer_free(void* lexer)
{
    delete reinterpret_cast<BuiltinLexer*>(lexer);
}

void* builtin_lexer_instance_new(const void* lexer_parent, const char* input, size_t length)
{
    return new BuiltinLexerSession(reflex::Input(input, length),
                                   reinterpret_cast<const BuiltinLexer*>(lexer_parent));
}

void builtin_lexer_instance_free(void* lexer_inst)
{
    delete reinterpret_cast<BuiltinLexerSession*>(lexer_inst);
}

int builtin_lexer_next(void* lexer, void* ll_val)
{
    return reinterpret_cast<BuiltinLexerSession*>(lexer)->next(ll_val);
}

