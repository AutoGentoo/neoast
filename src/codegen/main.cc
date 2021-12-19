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


#include <util/util.h>
#include <iostream>
#include <fstream>
#include "codegen/codegen.h"
#include "input_file.h"
#include "codegen_priv.h"

int main(int argc, const char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s [INPUT_FILE] [OUTPUT_FILE].cc? [OUTPUT_FILE].h\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_c = argv[2];
    const char* output_h = argv[3];

    try
    {
        InputFile input(input_file);
        CodeGen cg(input.file, input.full_path);

        if (has_errors()) return 1;

        std::ofstream h(output_h);
        cg.write_header(h);
        h.close();

        if (has_errors()) return 2;

        std::ofstream c(output_c);
        cg.write_source(c);
        c.close();

        if (has_errors()) return 3;
    }
    catch (const ASTException &e)
    { emit_error(e.position(), e.what()); }
    catch (const Exception &e)
    { emit_error(nullptr, e.what()); }
    catch (const std::exception &e)
    { emit_error(nullptr, "System exception: %s", e.what()); }

    return (int)has_errors();
}
