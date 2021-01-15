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
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s [INPUT_FILE] [OUTPUT_FILE]\n", argv[0]);
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
    fclose(fp);

    GrammarParser parser;
    if (gen_parser_init(&parser))
        return 1;

    ParserBuffers* buf = parser_allocate_buffers(1024, 1024, 16, sizeof(CodegenUnion));
    int tok_n = lexer_fill_table(input, file_size, &parser, buf);

    printf("tokens: %d\n", tok_n);
    I32 result_idx = parser_parse_lr(&parser, GEN_parsing_table, buf);

    if (result_idx == -1)
    {
        fprintf(stderr, "Failed to parse file '%s'\n", argv[1]);
        return 1;
    }

    fp = fopen(argv[2], "w+");
    if (!fp)
    {
        fprintf(stderr, "Failed to open '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    struct File* f = ((CodegenUnion*)buf->value_table)[result_idx].file;
    codegen_write(f, fp);
    fclose(fp);

    parser_free_buffers(buf);
    parser_free(&parser);
    free(input);
    file_free(f);
    return 0;
}