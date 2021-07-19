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


#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <codegen/builtin_lexer/builtin_lexer.h>
#include <stddef.h>
#include "codegen/codegen.h"
#include "codegen/compiler.h"
#include "grammar.h"

extern uint32_t* GEN_parsing_table;

int main(int argc, const char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s [INPUT_FILE] [OUTPUT_FILE].cc? [OUTPUT_FILE].h\n", argv[0]);
        return 1;
    }

    char* input;
    FILE* fp = fopen(argv[1], "r");
    if (!fp)
    {
        fprintf(stderr, "Failed to open '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Read the whole file
    input = malloc(file_size + 1);
    size_t offset = 0;
    while ((offset += fread(input + offset, 1, 1024, fp)) < file_size);
    input[file_size] = 0;
    fclose(fp);

    GrammarParser parser;
    void* lexer = NULL;
    if (gen_parser_init(&parser, &lexer))
    {
        free(input);
        return 1;
    }

    ParserBuffers* buf = parser_allocate_buffers(
            256, 256,
            sizeof(CodegenStruct), offsetof(CodegenStruct, position));
    void* lexer_instance = builtin_lexer_instance_new(lexer, input, file_size);

    int32_t result_idx = parser_parse_lr(&parser, GEN_parsing_table, buf,
                                         lexer_instance, builtin_lexer_next);

    builtin_lexer_instance_free(lexer_instance);
    builtin_lexer_free(lexer);

    if (result_idx == -1)
    {
        fprintf(stderr, "Failed to parse file '%s'\n", argv[1]);
        parser_free_buffers(buf);
        free(input);
        return 1;
    }

    struct File* f = ((CodegenUnion*)buf->value_table)[result_idx].file;

    TokenPosition p = {0, 0};
    KeyVal* builtin_includes = declare_include(&p,
                                               strdup("#include <codegen/codegen.h>\n"
                                                      "#include <codegen/compiler.h>"));
    builtin_includes->next = f->header;
    f->header = builtin_includes;

    int error = codegen_write(NULL, f, argv[2], argv[3]);

    parser_free_buffers(buf);
    free(input);
    file_free(f);

    return error;
}
