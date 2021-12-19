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


#include <parsergen/canonical_collection.h>
#include <sstream>
#include "util.h"

static inline void dump_name(const char* t_name, std::ostream& os)
{
#define MAPPING(ascii, html) \
    case (ascii): \
        os << (html); \
        break

    for (const char* iter = t_name; *iter; iter++)
    {
        switch(*iter)
        {
            MAPPING('\\', "&bsol;");
            MAPPING('"', "&quot;");
            MAPPING('\'', "&apos;");
            MAPPING('&', "&amp;");
            default:
                os << *iter;
        }
    }
#undef MAPPING
}

static void dump_graphviz_state(
        const parsergen::CanonicalCollection* cc,
        const parsergen::GrammarState* gs,
        std::ostream &os,
        bool full)
{
    if (!full)
    {
        os << "s" << gs->get_id()
           << "shape=none, margin=0, label=<s" << gs->get_id() << ">]\n";
        return;
    }

    os << "s" << gs->get_id()
       << " [shape=none, margin=0, label=<\n"
          "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n"
          "    <TR><TD>s" << gs->get_id() << "</TD></TR>\n" // header
                                             "    <TR><TD>\n";

    for (const auto &lr1: *gs)
    {
        // Place production
        os << cc->parser()->token_names[cc->to_index(lr1.derivation->token)] << " &rarr;";
        for (uint32_t i = 0; i < lr1.derivation->tok_n; i++)
        {
            os << " ";
            if (i == lr1.i) os << "&bull; ";
            dump_name(cc->parser()->token_names[cc->to_index(lr1.derivation->grammar[i])], os);
        }

        if (lr1.is_final()) os << " &bull;";

        // Place lookaheads
        os << " (";
        std::string sep;
        for (const auto &idx: lr1.look_ahead)
        {
            os << sep;
            dump_name(cc->parser()->token_names[idx], os);
            sep = ", ";
        }
        os << ")<br/>\n";
    }
    os << "    </TD></TR>\n"
          "    </TABLE>>]\n";
}

static void dump_graphviz_state_edges(
        const parsergen::CanonicalCollection* cc,
        const parsergen::GrammarState* gs,
        std::ostream &os)
{
    for (auto iter = gs->dfa_begin(); iter != gs->dfa_end(); iter++)
    {
        os << "s" << gs->get_id() << " -> s" << iter->second->get_id()
           << " [taillabel=\"";
           dump_name(cc->parser()->token_names[cc->to_index(iter->first)], os);
           os << "\"]\n";
    }
}

void dump_graphviz_cxx(
        const parsergen::CanonicalCollection* cc,
        std::ostream &os,
        bool full)
{
    os << "digraph{\n";

    // Dump all state nodes
    for (uint32_t st_i = 0; st_i < cc->size(); st_i++)
    {
        dump_graphviz_state(cc, cc->get_state(st_i), os, full);
    }

    // Connect the states with transitions
    for (uint32_t st_i = 0; st_i < cc->size(); st_i++)
    {
        dump_graphviz_state_edges(cc, cc->get_state(st_i), os);
    }

    os << "}\n";
}

void dump_graphviz(const void* cc, FILE* fp)
{
    std::ostringstream os_fp;

    dump_graphviz_cxx(static_cast<const parsergen::CanonicalCollection*>(cc), os_fp);
    std::string s = os_fp.str();

    fwrite(s.c_str(), s.length(), 1, fp);
}
