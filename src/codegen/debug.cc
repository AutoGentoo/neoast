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
#include <fstream>
#include "codegen_priv.h"
#include "codegen_impl.h"
#include "input_file.h"

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
            {"input",       required_argument, nullptr, 'i'},
            {"small-graph", no_argument,       nullptr, 'q'},
            {"help",        no_argument,       nullptr, 'h'},
            {nullptr, 0,                       nullptr, 0}
    };

    const char* graph_file = nullptr;
    const char* table_file = nullptr;
    const char* states_file = nullptr;
    const char* input_file = nullptr;
    bool small_graph = false;

    int rc, option_index = 0;
    int error = 0, help = 0;
    while ((rc = getopt_long_only(argc, argv, "g:t:qs:hi:",
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
            case 'i':
                input_file = optarg;
                break;
            case 'h':
                help = 1;
                break;
            case 'q':
                small_graph = true;
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

    if (!input_file)
    {
        std::cout << "an input file must be provided (--input=FILE)\n";
        error = 2;
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
                     "Options:\n"
                     "  -i, --input             (required) parser input file to debug\n"
                     "  -h, --help              generate this message and exit\n"
                     "  -q, --small-graph       used with --graph to generate smaller graphs for large parsers\n";
        return error;
    }

    // Parse and generate the canonical collection
    InputFile input(input_file);
    if (has_errors())
    {
        input.put_errors();
        return (int)has_errors();
    }

    do
    {
        try
        {
            CodeGen cg(input.file, input.full_path);
            if (has_errors()) break;

            const parsergen::CanonicalCollection* cc = cg.get_impl()->cc.get();
            if (graph_file)
            {
                std::ofstream gf(graph_file);
                dump_graphviz_cxx(cc, gf, small_graph);
                gf.close();
            }

            if (table_file)
            {
                std::ofstream tf(table_file);
                up<uint32_t[]> table = up<uint32_t[]>(new uint32_t[cc->table_size()]);
                cc->generate(table.get(), cg.get_impl()->precedence_table.get());
                dump_table_cxx(table.get(), cc, cg.get_impl()->token_names_ptr.get(),
                               0, tf, "");
                tf.close();
            }

            if (states_file)
            {
                std::ofstream sf(states_file);
                dump_states_cxx(cc, sf);
                sf.close();
            }
        }
        catch (const ASTException &e)
        { emit_error(e.position(), e.what()); }
        catch (const Exception &e)
        { emit_error(nullptr, e.what()); }
        catch (const std::exception &e)
        { emit_error(nullptr, "System exception: %s", e.what()); }
    } while (false);

    input.put_errors();
    return (int)has_errors();
}
