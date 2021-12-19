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

#include <reflex/convert.h>
#include "cg_neoast_lexer.h"
#include "cg_pattern.h"

#define REFLEX_SIGNATURE "imsx#=^:abcdefhijklnrstuvwxzABDHLNQSUW<>?."
#define REFLEX_FLAGS reflex::convert_flag::freespace

struct CGNeoastLexerState;

struct CGNeoastLexerRule
{
    Code code;
    std::string regex;

    explicit CGNeoastLexerRule(
            const LexerRuleProto* iter,
            const MacroEngine &m_engine)
            : code(iter->function, &iter->position)
    {
        try
        {
            // Makes this regular expression compatible for
            // use with the combined state DFA. All capture
            // groups are converted to non-capturing: (?:regex_group)
            // This will allow us to make a single regex rule:
            //     (?mx)((?:rule1))|((?:rule2)) etc.
            // TODO(tumbar) Integrate a system to enable flags setting
            regex = reflex::convert(iter->regex,
                                    REFLEX_SIGNATURE,
                                    REFLEX_FLAGS,
                                    &m_engine.get_original());
            regex = m_engine.expand(regex);
        }
        catch (reflex::regex_error &e)
        {
            emit_error(&iter->position,
                       "malformed regular expression or unsupported syntax: '%s', regex: '%s'",
                       e.what(),
                       iter->regex);
        }
    }
};

struct CGNeoastLexerState
{
    std::string name;
    std::vector<CGNeoastLexerRule> rules;

    std::string cached_regex;
    size_t cached_regex_len;

    explicit CGNeoastLexerState(std::string name)
            : name(std::move(name)), cached_regex_len(0)
    {
    }

    const std::string &get_name() const
    { return name; }

    void add_rule(const LexerRuleProto* iter_s,
                  const MacroEngine &m_engine)
    {
        rules.emplace_back(iter_s, m_engine);
    }

    const std::string &get_pattern()
    {
        // Only regenerate the regular expression if we need to
        if (cached_regex_len < rules.size())
        {
            // Regenerate the cache
            std::stringstream ss;
            ss << "(?mx)";
            std::string split_s;
            for (const auto &rule: rules)
            {
                ss << split_s << "((?:" << rule.regex << "))";
                split_s = "|";
            }

            cached_regex = ss.str();
        }

        return cached_regex;
    }
};

struct CGNeoastLexerImpl
{
    std::vector<CGNeoastLexerState> states;
    const Options &options;

    explicit CGNeoastLexerImpl(const Options &options) : options(options)
    {}
};

CGNeoastLexer::CGNeoastLexer(const File* self,
                             const MacroEngine &m_engine_,
                             const Options &options)
        : impl_(new CGNeoastLexerImpl(options))
{
    impl_->states.emplace_back("LEX_STATE_DEFAULT");

    for (const LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        if (iter->lexer_state)
        {
            // Make sure the name does not overlap
            bool duplicate_state = false;
            for (const auto &iter_state: impl_->states)
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

            impl_->states.emplace_back(iter->lexer_state);
            auto &ls = impl_->states[impl_->states.size() - 1];

            for (const LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
            {
                assert(iter_s->regex);
                assert(iter_s->function);
                assert(!iter_s->lexer_state);
                assert(!iter_s->state_rules);

                ls.add_rule(iter_s, m_engine_);
            }
        }
        else
        {
            // Add to the default lexing state
            impl_->states[0].add_rule(iter, m_engine_);
        }
    }
}

void CGNeoastLexer::put_top(std::ostream &os) const
{
    // Lexer includes
    os << "#include <lexer/matcher_fsm.h>\n"
          "#include <lexer/matcher.h>\n\n";

    // Build the enum of state ids
    os << "enum {\n";
    for (const auto &state: impl_->states)
    {
        os << "    " << state.name << ",\n";
    }
    os << "};\n";
}

void CGNeoastLexer::put_global(std::ostream &os) const
{
    for (auto state: impl_->states)
    {
        cg_pattern_put_fsm(
                os,
                state.get_pattern(),
                state.name + "_FSM");
        os << "\n";
    }

    // Put the lexing function
    os << "static int neoast_lexer_next(NeoastMatcher* self__, NeoastValue* destination__, void* context__)\n"
          "{\n"
          ""// Lexing macros\n"
          "#define yyval (&(destination__->value))\n"
          "#define yystate (self__->lexing_state)\n"
          "#define yypush(state) NEOAST_STACK_PUSH(yystate, (state))\n"
          "#define yypop() NEOAST_STACK_POP(yystate)\n"
          "#define yyposition (&(destination__->position))\n"
          "#define yycontext (context__)\n"
          "#define yylen (self__->len_)\n\n"
          "    while (!matcher_at_end(self__))\n"
          "    {\n"
          "        switch (NEOAST_STACK_PEEK(yystate))\n"
          "        {\n";

    for (const auto &state: impl_->states)
    {
        os <<
           "        case " << state.name << ":\n"
                                            "        {\n"
                                            "            size_t neoast_tok___ = matcher_scan(self__, " << state.name
           << "_FSM);\n"
              "            const char* yytext = matcher_text(self__);\n"
              "            yyposition->line = matcher_lineno(self__);\n"
              "            yyposition->col = matcher_columno(self__);\n"
              "            yyposition->len = matcher_size(self__);\n\n"
              "            switch(neoast_tok___)\n"
              "            {\n"
              "            default:\n"
              "            case 0:\n";
        if (get_options().lexing_error_cb.empty())
        {
            os << "                if (!matcher_at_end(self__)) { fprintf(stderr, \"Failed to match token near "
                  "line:col %d:%d (state " << state.name
               << ")\", yyposition->line, yyposition->col_start); return -1; }\n"
                  "                else return 0;\n";
        }
        else
        {
            os << "                if (!matcher_at_end(self__)) { " << get_options().lexing_error_cb
               << "(yycontext, yytext, yyposition, \"" << state.name << "\"); return -1; }\n"
                                                             "                else return 0;\n";
        }

        int i = 0;
        for (const auto &rule: state.rules)
        {
            os <<
               "            case " << ++i << ":\n {"
               << rule.code.get_simple(get_options()) << "}\n"
               << "                break;\n";
        }

        os <<
           "            }\n"
           "\n"
           "        }\n"
           "        break;\n";
    }
    os <<
       "    }}\n"
       "\n"
       "    // EOF\n"
       "    return 0;\n"
       "#undef yyval\n"
       "#undef yystate\n"
       "#undef yypush\n"
       "#undef yypop\n"
       "#undef yyposition\n"
       "#undef yycontext\n"
       "#undef yylen\n"
       "}\n";

}
void CGNeoastLexer::put_bottom(std::ostream& os) const
{
    // Write the wrapper function
    os << "static int neoast_lexer_next_wrapper(void* matcher, void* dest, void* error_ctx)\n"
          "{\n"
          "    int tok = neoast_lexer_next((NeoastMatcher*)matcher, (NeoastValue*)dest, error_ctx);\n\n"
          "    // EOF and INVALID\n"
          "    if (tok <= 0) return tok;\n\n"
          "    if (tok < NEOAST_ASCII_MAX)\n"
          "    {\n"
          "         int t_tok = (int32_t)neoast_ascii_mappings[tok];\n"
          "         if (t_tok <= NEOAST_ASCII_MAX)\n"
          "         {\n"
          "             fprintf(stderr, \"Lexer return '%c' (%d) which has not been explicitly defined as a token\",\n"
          "                     tok, tok);\n"
          "             fflush(stderr);\n"
          "             exit(1);\n"
          "         }\n\n"
          "         return t_tok - NEOAST_ASCII_MAX;\n"
          "    }\n"
          "    return tok - NEOAST_ASCII_MAX;\n"
          "}";
}

const Options &CGNeoastLexer::get_options() const
{
    return impl_->options;
}

std::string CGNeoastLexer::get_init() const
{
    return "";
}

std::string CGNeoastLexer::get_delete() const
{
    return "";
}

CGNeoastLexer::~CGNeoastLexer()
{
    delete impl_;
}

std::string CGNeoastLexer::get_new_inst(const std::string &name) const
{
    return "NeoastMatcher* ll_inst = matcher_new(input);\n"
           "    NEOAST_STACK_PUSH(ll_inst->lexing_state, LEX_STATE_DEFAULT);";
}

std::string CGNeoastLexer::get_del_inst(const std::string &name) const
{
    return "matcher_free(ll_inst);";
}

std::string CGNeoastLexer::get_ll_next(const std::string &name) const
{
    return "neoast_lexer_next_wrapper";
}
