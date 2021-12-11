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

#include <reflex/timer.h>
#include "cg_pattern.h"
#include "cg_util.h"

#define WITH_COMPACT_DFA (-1)

namespace reflex
{
    /**
     * This is called reflex::NeoastMatcher to take advantage
     * of the fact that the actual reflex::NeoastMatcher is a
     * friend to reflex::Pattern
     */
    class NeoastPattern : public Pattern
    {
    private:
        void gencode_dfa(
                std::ostream &os,
                const Pattern::DFA::State* start,
                const std::string &func_name);

        void
        gencode_dfa_closure(
                std::ostream &os,
                const Pattern::DFA::State* state, int nest,
                bool peek);

    public:
        explicit NeoastPattern(const std::string &regex)
        {
            rex_ = regex;
            fsm_ = nullptr;
            opc_ = nullptr;

            // raise syntax errors, optimized FSM, multiline matching
            init_options("rom");
            nop_ = 0;
            len_ = 0;
            min_ = 0;
            one_ = false;
        }

        void custom_codegen(std::ostream &os,
                            const std::string &func_name)
        {
            Positions startpos;
            Follow followpos;
            Map modifiers;
            Map lookahead;
            // parse the regex pattern to construct the followpos NFA without epsilon transitions
            parse(startpos, followpos, modifiers, lookahead);
            // start state = startpos = firstpost of the followpos NFA, also merge the tree DFA root when non-NULL
            DFA::State* start = dfa_.state(tfa_.tree, startpos);
            // compile the NFA into a DFA
            compile(start, followpos, modifiers, lookahead);
            // assemble DFA opcode tables or direct code
            timer_type t;
            timer_start(t);
            predict_match_dfa(start);
            export_dfa(start);
            compact_dfa(start);
            encode_dfa(start);
            wms_ = timer_elapsed(t);
            gencode_dfa(os, start, func_name);
            // delete the DFA
            dfa_.clear();
        }
    };

    static const char* meta_label[] = {
            nullptr,
            "NWB",
            "NWE",
            "BWB",
            "EWB",
            "BWE",
            "EWE",
            "BOL",
            "EOL",
            "BOB",
            "EOB",
            "UND",
            "IND",
            "DED",
    };

    static void print_char(std::ostream &os, int c, bool h = false)
    {
        if (c >= '\a' && c <= '\r')
            os << variadic_string("'\\%c'", "abtnvfr"[c - '\a']);
        else if (c == '\\')
            os << "'\\\\'";
        else if (c == '\'')
            os << "'\\''";
        else if (std::isprint(c))
            os << variadic_string("'%c'", c);
        else if (h)
            os << variadic_string("%02x", c);
        else
            os << variadic_string("%u", c);
    }

    void NeoastPattern::gencode_dfa(std::ostream &os,
                                    const Pattern::DFA::State* start,
                                    const std::string &func_name)
    {
        os << variadic_string("static void %s(NeoastMatcher* m)\n"
                              "{\n"
                              "  int c0, c1 = 0;\n"
                              "  FSM_INIT(m, &c1);\n",
                              func_name.c_str());
        for (const Pattern::DFA::State* state = start; state; state = state->next)
        {
            os << variadic_string("\nS%u:\n", state->index);
            if (state == start)
                os << "  FSM_FIND(m);\n";
            if (state->redo)
                os << "  FSM_REDO(m, EOF);\n";
            else if (state->accept > 0)
                os << variadic_string("  FSM_TAKE(m, %u, EOF);\n", state->accept);
            for (unsigned short tail: state->tails)
                os << variadic_string("  FSM_TAIL(m, %u);\n", tail);
            for (unsigned short head: state->heads)
                os << variadic_string("  FSM_HEAD(m, %u);\n", head);
            if (state->edges.rbegin() != state->edges.rend() && state->edges.rbegin()->first == Pattern::META_DED)
                os << variadic_string("  if (FSM_DENT(m)) goto S%u;\n", state->edges.rbegin()->second.second->index);
            bool peek = false; // if we need to read a character into c1
            bool prev = false; // if we need to keep the previous character in c0
            for (auto i = state->edges.rbegin(); i != state->edges.rend(); ++i)
            {
#if WITH_COMPACT_DFA == -1
                Pattern::Char lo = i->first;
                    Pattern::Char hi = i->second.first;
#else
                Pattern::Char hi = i->first;
                Pattern::Char lo = i->second.first;
#endif
                if (!Pattern::is_meta(lo))
                {
                    Pattern::Index target_index = Pattern::Const::IMAX;
                    if (i->second.second != NULL)
                        target_index = i->second.second->index;
                    auto j = i;
                    if (target_index == Pattern::Const::IMAX &&
                        (++j == state->edges.rend() || Pattern::is_meta(j->second.first)))
                        break;
                    peek = true;
                }
                else
                {
                    do
                    {
                        if (lo == Pattern::META_EOB || lo == Pattern::META_EOL)
                            peek = true;
                        else if (lo == Pattern::META_EWE || lo == Pattern::META_BWE || lo == Pattern::META_NWE)
                            prev = peek = true;
                        if (prev && peek)
                            break;
                        check_dfa_closure(i->second.second, 1, peek, prev);
                    } while (++lo <= hi);
                }
            }
            bool read = peek;
            bool elif = false;
#if WITH_COMPACT_DFA == -1
            for (auto i = state->edges.rbegin(); i != state->edges.rend(); ++i)
                {
                    Pattern::Char lo = i->first;
                    Pattern::Char hi = i->second.first;
                    Pattern::Index target_index = Pattern::Const::IMAX;
                    if (i->second.second != NULL)
                        target_index = i->second.second->index;
                    if (read)
                    {
                        if (prev)
                            os << "  c0 = c1, c1 = FSM_CHAR(m);\n";
                        else
                            os << "  c1 = FSM_CHAR(m);\n";
                        read = false;
                    }
                    if (!Pattern::is_meta(lo))
                    {
                        auto j = i;
                        if (target_index == Pattern::Const::IMAX && (++j == state->edges.rend() || Pattern::is_meta(j->second.first)))
                            break;
                        if (lo == hi)
                        {
                            os << "  if (c1 == ";
                            print_char(os, lo);
                            os << ")";
                        }
                        else if (hi == 0xFF)
                        {
                            os << "  if (";
                            print_char(os, lo);
                            os << " <= c1)";
                        }
                        else
                        {
                            os << "  if (";
                            print_char(os, lo);
                            os << " <= c1 && c1 <= ";
                            print_char(os, hi);
                            os << ")";
                        }
                        if (target_index == Pattern::Const::IMAX)
                        {
                            if (peek)
                                os << " return FSM_HALT(m, c1);\n";
                            else
                                os << " return FSM_HALT(m, CONST_UNK);\n";
                        }
                        else
                        {
                            os << variadic_string(" goto S%u;\n", target_index);
                        }
                    }
                    else
                    {
                        do
                        {
                            switch (lo)
                            {
                                case Pattern::META_EOB:
                                case Pattern::META_EOL:
                                    os << "  ";
                                    if (elif)
                                        os << "else ";
                                    os << variadic_string("if (FSM_META_%s(m, c1)) {\n", meta_label[lo - Pattern::META_MIN]);
                                    gencode_dfa_closure(os, i->second.second, 2, peek);
                                    os << "  }\n";
                                    elif = true;
                                    break;
                                case Pattern::META_EWE:
                                case Pattern::META_BWE:
                                case Pattern::META_NWE:
                                    os << "  ";
                                    if (elif)
                                        os << "else ";
                                    os << variadic_string("if (FSM_META_%s(m, c0, c1)) {\n", meta_label[lo - Pattern::META_MIN]);
                                    gencode_dfa_closure(os, i->second.second, 2, peek);
                                    os << "  }\n";
                                    elif = true;
                                    break;
                                default:
                                    os << "  ";
                                    if (elif)
                                        os << "else ";
                                    os << variadic_string("if (FSM_META_%s(m)) {\n", meta_label[lo - Pattern::META_MIN]);
                                    gencode_dfa_closure(os, i->second.second, 2, peek);
                                    os << "  }\n";
                                    elif = true;
                            }
                        } while (++lo <= hi);
                    }
                }
#else
            for (auto i = state->edges.rbegin(); i != state->edges.rend(); ++i)
            {
                Pattern::Char hi = i->first;
                Pattern::Char lo = i->second.first;
                if (Pattern::is_meta(lo))
                {
                    if (read)
                    {
                        if (prev)
                            os << "  c0 = c1, c1 = FSM_CHAR(m);\n";
                        else
                            os << "  c1 = FSM_CHAR(m);\n";
                        read = false;
                    }
                    do
                    {
                        switch (lo)
                        {
                            case Pattern::META_EOB:
                            case Pattern::META_EOL:
                                os << "  ";
                                if (elif)
                                    os << "else ";
                                os << variadic_string("if (m.FSM_META_%s(c1)) {\n", meta_label[lo - Pattern::META_MIN]);
                                gencode_dfa_closure(os, i->second.second, 2, peek);
                                os << "  }\n";
                                elif = true;
                                break;
                            case Pattern::META_EWE:
                            case Pattern::META_BWE:
                            case Pattern::META_NWE:
                                os << "  ";
                                if (elif)
                                    os << "else ";
                                os << variadic_string("if (m.FSM_META_%s(c0, c1)) {\n",
                                                      meta_label[lo - Pattern::META_MIN]);
                                gencode_dfa_closure(os, i->second.second, 2, peek);
                                os << "  }\n";
                                elif = true;
                                break;
                            default:
                                os << "  ";
                                if (elif)
                                    os << "else ";
                                os << variadic_string("if (m.FSM_META_%s()) {\n", meta_label[lo - Pattern::META_MIN]);
                                gencode_dfa_closure(os, i->second.second, 2, peek);
                                os << "  }\n";
                                elif = true;
                        }
                    } while (++lo <= hi);
                }
            }
            for (auto i = state->edges.begin(); i != state->edges.end(); ++i)
            {
                Pattern::Char hi = i->first;
                Pattern::Char lo = i->second.first;
                Pattern::Index target_index = Pattern::Const::IMAX;
                if (i->second.second != NULL)
                    target_index = i->second.second->index;
                if (read)
                {
                    if (prev)
                        os << "  c0 = c1, c1 = FSM_CHAR(m);\n";
                    else
                        os << "  c1 = FSM_CHAR(m);\n";
                    read = false;
                }
                if (!Pattern::is_meta(lo))
                {
                    auto j = i;
                    if (target_index == Pattern::Const::IMAX &&
                        (++j == state->edges.end() || Pattern::is_meta(j->second.first)))
                        break;
                    if (lo == hi)
                    {
                        os << "  if (c1 == ";
                        print_char(os, lo);
                        os << ")";
                    }
                    else if (hi == 0xFF)
                    {
                        os << "  if (";
                        print_char(os, lo);
                        os << " <= c1)";
                    }
                    else
                    {
                        os << "  if (";
                        print_char(os, lo);
                        os << " <= c1 && c1 <= ";
                        print_char(os, hi);
                        os << ")";
                    }
                    if (target_index == Pattern::Const::IMAX)
                    {
                        if (peek)
                            os << " return FSM_HALT(m, c1);\n";
                        else
                            os << " return FSM_HALT(m, CONST_UNK);\n";
                    }
                    else
                    {
                        os << variadic_string(" goto S%u;\n", target_index);
                    }
                }
            }
#endif
            if (peek)
                os << "  return FSM_HALT(m, c1);\n";
            else
                os << "  return FSM_HALT(m, CONST_UNK);\n";
        }
        os << "}\n\n";
    }

    void NeoastPattern::gencode_dfa_closure(std::ostream &os, const Pattern::DFA::State* state,
                                            int nest, bool peek)
    {
        bool elif = false;
        if (state->redo)
        {
            if (peek)
                os << variadic_string("%*sFSM_REDO(m, c1);\n", 2 * nest, "");
            else
                os << variadic_string("%*sFSM_REDO(m, EOF);\n", 2 * nest, "");
        }
        else if (state->accept > 0)
        {
            if (peek)
                os << variadic_string("%*sFSM_TAKE(m, %u, c1);\n", 2 * nest, "", state->accept);
            else
                os << variadic_string("%*sFSM_TAKE(m, %u, EOF);\n", 2 * nest, "", state->accept);
        }
        for (unsigned short tail: state->tails)
            os << variadic_string("%*sFSM_TAIL(m, %u);\n", 2 * nest, "", tail);
        if (nest > 5)
            return;
        for (auto i = state->edges.rbegin(); i != state->edges.rend(); ++i)
        {
#if WITH_COMPACT_DFA == -1
            Pattern::Char lo = i->first;
                Pattern::Char hi = i->second.first;
#else
            Pattern::Char hi = i->first;
            Pattern::Char lo = i->second.first;
#endif
            if (Pattern::is_meta(lo))
            {
                do
                {
                    switch (lo)
                    {
                        case Pattern::META_EOB:
                        case Pattern::META_EOL:
                            os << variadic_string("%*s", 2 * nest, "");
                            if (elif)
                                os << "else ";
                            os << variadic_string("if (m.FSM_META_%s(c1)) {\n", meta_label[lo - Pattern::META_MIN]);
                            gencode_dfa_closure(os, i->second.second, nest + 1, peek);
                            os << variadic_string("%*s}\n", 2 * nest, "");
                            elif = true;
                            break;
                        case Pattern::META_EWE:
                        case Pattern::META_BWE:
                        case Pattern::META_NWE:
                            os << variadic_string("%*s", 2 * nest, "");
                            if (elif)
                                os << "else ";
                            os << variadic_string("if (m.FSM_META_%s(c0, c1)) {\n", meta_label[lo - Pattern::META_MIN]);
                            gencode_dfa_closure(os, i->second.second, nest + 1, peek);
                            os << variadic_string("%*s}\n", 2 * nest, "");
                            elif = true;
                            break;
                        default:
                            os << variadic_string("%*s", 2 * nest, "");
                            if (elif)
                                os << "else ";
                            os << variadic_string("if (m.FSM_META_%s()) {\n", meta_label[lo - Pattern::META_MIN]);
                            gencode_dfa_closure(os, i->second.second, nest + 1, peek);
                            os << variadic_string("%*s}\n", 2 * nest, "");
                            elif = true;
                    }
                } while (++lo <= hi);
            }
        }
    }
}

void cg_pattern_put_fsm(std::ostream &os,
                        const std::string &regex,
                        const std::string &func_name)
{
    reflex::NeoastPattern(regex).custom_codegen(os, func_name);
}
