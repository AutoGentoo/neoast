/*
 * $ORGANIZATION_NAME
 * Copyright (C) 2021  AutoGentoo
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <getopt.h>
#include <cstdint>
#include <iostream>
#include "codegen_priv.h"

uint32_t cc_init();

void cc_free();

void* cc_allocate_buffers();

void cc_free_buffers(void* self);

struct File* cc_parse_len(void* buffers, const char* input, uint64_t input_len);

int main(int argc, char* argv[])
{
    struct option long_options[] = {
            {"graph",       required_argument, nullptr, 'g'},
            {"table",       required_argument, nullptr, 't'},
            {"states",      required_argument, nullptr, 's'},
            {"small-graph", no_argument,       nullptr, 'q'},
            {"help",        no_argument,       nullptr, 'h'},
            {nullptr, 0,                       nullptr, 0}
    };

    const char* graph_file = nullptr;
    const char* table_file = nullptr;
    const char* states_file = nullptr;
    bool small_graph = false;

    int rc, option_index = 0;
    int error = 0, help = 0;
    while ((rc = getopt_long_only(argc, argv, "g:t:qs:h",
                                  long_options, &option_index)) != -1)
    {
        switch (rc)
        {
            case 'g':
                graph_file = optarg;
                break;
            case 't':
                table_file = optarg;
                break;
            case 's':
                states_file = optarg;
                break;
            case 'h':
                help = 1;
                break;
            case 0 :  /* no short form case  */
            case '?':  /* Handled by the default error handler */
                break;
            default:
                std::cout << "undefined option " << (char) rc;
                error++;
                break;
        }
    }

    if (!graph_file && !table_file && !states_file)
    {
        std::cout << "at least one of --graph, --table, or --states is required\n";
        error = 1;
    }

    if (error || help)
    {
        std::cout << "Usage: neoast-debug [OPTION]... -i input.y\n"
                     "Generate debugging information about a generated parser.\n"
                     "Parses and resolves input file\n\n"
                     "Generation types, choose at least one:\n"
                     "  -g, --graph=FILE        graphviz dot file\n"
                     "  -t, --table=FILE        LALR(1)/CLR(1) parsing table with debugging information\n"
                     "  -s, --states=FILE       similar to graphviz but in human readable ascii\n\n"
                     "Options:"
                     "  -h, --help              generate this message and exit\n"
                     "  -q, --small-graph       used with --graph to generate smaller graphs for large parsers\n";
        return error;
    }

    // Parse and generate the canonical collection
    CodeGen cg()

    if (graph_file)
    {

    }

    return 0;
}
