//
// Created by tumbar on 12/24/20.
//

#include <parser.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "codegen.h"

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [INPUT_FILE]\n", argv[0]);
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
    assert(fread(input, 1, file_size, fp) == file_size);
    input[file_size] = 0;

    GrammarParser parser;
    if (gen_parser_init(&parser))
        return 1;

    U32 token_table[1024];
    CodegenUnion value_table[1024];

    int tok_n = lexer_fill_table(input, file_size, &parser, token_table,
                                 value_table, sizeof(CodegenUnion), 1024);

    printf("tokens: %d\n", tok_n);
    Stack* p_stack = parser_allocate_stack(1024);
    I32 result_idx = parser_parse_lr(&parser, p_stack, GEN_parsing_table, tok_names_errors,
                                     token_table, value_table, sizeof(CodegenUnion));

    if (result_idx == -1)
    {
        fprintf(stderr, "Failed to parse file '%s'\n", argv[1]);
        return 1;
    }
    struct File* f = value_table[result_idx].file;

    parser_free_stack(p_stack);
    parser_free(&parser);
    free(input);
    return 0;
}