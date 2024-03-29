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
#include "bootstrap_lexer.h"
#include <reflex/pattern.h>
#include <reflex/matcher.h>
#include <reflex/abslexer.h>
#include <sstream>


struct BootstrapLexerState
{
    std::vector<const LexerRule*> rules;
    std::string regex_str;
    reflex::Pattern pattern;

    BootstrapLexerState(const LexerRule* rules_,
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
        for (const auto* rule: rules)
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


struct BootstrapLexer
{
    std::vector<BootstrapLexerState> states;
    ll_error_cb error_cb;
    size_t position_offset;
    const uint32_t* ascii_mappings;

    BootstrapLexer(const LexerRule* const* rules,
                   const uint32_t rules_n[],
                   uint32_t state_n,
                   ll_error_cb error,
                   size_t position_offset,
                   const uint32_t* ascii_mappings)
            : error_cb(error), position_offset(position_offset),
              ascii_mappings(ascii_mappings)
    {
        assert(state_n);
        states.reserve(state_n);

        for (int i = 0; i < state_n; i++)
        {
            states.emplace_back(rules[i], rules_n[i]);
        }
    }
};


class BootstrapLexerSession : public reflex::AbstractLexer<reflex::Matcher>
{
    const BootstrapLexer* parent;
    const reflex::Input input;
    ParsingStack* lexer_states;

public:
    BootstrapLexerSession(const char* input,
                          uint32_t length, const BootstrapLexer* parent,
                          std::ostream &os = std::cout) :
            AbstractLexer(input, os), parent(parent), input(input, length)
    {
        lexer_states = parser_allocate_stack(32);
        NEOAST_STACK_PUSH(lexer_states, 0);

        if (!has_matcher())
        {
            matcher(new Matcher(parent->states[0].pattern, stdinit(), this));
        }
    }

    int next(void* ll_val, void* context)
    {
        int tok = next_impl(ll_val, context);

        // EOF and INVALID
        if (tok <= 0) return tok;

        int t_tok;
        if (tok < NEOAST_ASCII_MAX)
        {
            t_tok = static_cast<int32_t>(parent->ascii_mappings[tok]);
            if (t_tok <= NEOAST_ASCII_MAX)
            {
                fprintf(stderr, "Lexer returned '%c' (%d) which has not been "
                                "explicitly defined as a token",
                        tok, tok);
                fflush(stdout);
                exit(1);
            }

            return t_tok - NEOAST_ASCII_MAX;
        }

        return tok - NEOAST_ASCII_MAX;
    }

    ~BootstrapLexerSession() override
    {
        parser_free_stack(lexer_states);
        lexer_states = nullptr;
    }

private:
    int next_impl(void* ll_val, void* context)
    {
        int tok = -1;
        while (tok < 0)
        {
            int current_state = NEOAST_STACK_PEEK(lexer_states);
            matcher().pattern(parent->states[current_state].pattern);
            const BootstrapLexerState &state = parent->states[current_state];

            auto rule_out = matcher().scan();
            if (rule_out == 0 && matcher().at_end())
            {
                // EOF
                return 0;
            }

            int rule_idx = static_cast<int>(rule_out) - 1;
            if (rule_idx >= state.rules.size())
            {
                // Invalid token
                TokenPosition p{
                        .line = static_cast<uint32_t>(lineno()),
                        .col=static_cast<uint16_t>(columno()),
                        .len=static_cast<uint16_t>(size())
                };

                if (parent->error_cb)
                {
                    parent->error_cb(context, input.cstring(), &p, std::to_string(current_state).c_str());
                }
                else
                {
                    std::cerr << "Failed to match token near on line:col "
                              << p.line << ":" << p.col << " (state " << current_state << ")\n";
                }

                return -1;
            }
            else
            {
                // Note where this token was found
                auto* position = reinterpret_cast<TokenPosition*>(
                        static_cast<char*>(ll_val) + parent->position_offset);
                position->line = static_cast<uint32_t>(lineno());
                position->col = static_cast<uint32_t>(columno());
                position->len = static_cast<uint32_t>(size());

                // Run token action
                if (state.rules[rule_idx]->tok)
                {
                    tok = state.rules[rule_idx]->tok;
                }
                else if (state.rules[rule_idx]->expr)
                {
                    tok = state.rules[rule_idx]->expr(text(), ll_val, size(),
                                                      lexer_states, position);
                }
            }
        }

        return tok;
    }
};


void* bootstrap_lexer_new(const LexerRule* rules[], const uint32_t rules_n[], uint32_t state_n,
                          ll_error_cb error_cb,
                          size_t position_offset, const uint32_t* ascii_mappings)
{
    return new BootstrapLexer(rules, rules_n, state_n, error_cb, position_offset, ascii_mappings);
}

void bootstrap_lexer_free(void* lexer)
{
    delete reinterpret_cast<BootstrapLexer*>(lexer);
}

void* bootstrap_lexer_instance_new(const void* lexer_parent,
                                   const char* input, size_t length)
{
    return new BootstrapLexerSession(input, length, reinterpret_cast<const BootstrapLexer*>(lexer_parent));
}

void bootstrap_lexer_instance_free(void* lexer_inst)
{
    delete reinterpret_cast<BootstrapLexerSession*>(lexer_inst);
}

int bootstrap_lexer_next(void* lexer, void* ll_val, void* context)
{
    return reinterpret_cast<BootstrapLexerSession*>(lexer)->next(ll_val, context);
}
