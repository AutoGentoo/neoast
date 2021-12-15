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
#include <assert.h>
#include <stdarg.h>
#include "codegen/codegen.h"

#define PATH_MAX 4096

#define ERROR_CONTEXT_LINE_N 3

uint32_t cc_init();

void cc_free();

void* cc_allocate_buffers();

void cc_free_buffers(void* self);

struct File* cc_parse_len(void* buffers, const char* input, uint64_t input_len);

static size_t line_n = 0;
static const char* path = NULL;
static const char** file_lines = NULL;
static int error_counter = 0;

static void put_position(const TokenPosition* self, const char* type)
{
    assert(file_lines);
    assert(type);
    assert(path);

    if (!self || self->line >= line_n)
    {
        fprintf(stderr, "%s: ", type);
        return;
    }

    // Get the number of digits in the line number:
    int dig_n = snprintf(NULL, 0, "%+d", self->line);

    for (int i = (int) self->line - ERROR_CONTEXT_LINE_N; i < (int) self->line; i++)
    {
        if (i < 0)
        {
            continue;
        }

        char* newline_pos = strchr(file_lines[i], '\n');
        if (newline_pos)
        {
            fprintf(stderr,
                    "% *d | %.*s\n", dig_n, i + 1,
                    (int) (newline_pos - file_lines[i]),
                    file_lines[i]);
        }
        else
        {
            fprintf(stderr, "% *d | %s\n", dig_n, i + 1, file_lines[i]);
        }
    }

    if (self->line > 0)
    {
        for (int j = 0; j < self->col_start + 3 + dig_n; j++)
        {
            fputc(' ', stderr);
        }

        fprintf(stderr, "^\n%s at %s:%d: ", type, path, self->line);
    }
    else
    {
        fprintf(stderr, "%s: ", type);
    }
}

static char* read_file(FILE* fp, size_t* file_size)
{
    fseek(fp, 0L, SEEK_END);
    *file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Read the whole file
    char* input = malloc(*file_size + 1);
    size_t offset = 0;
    while ((offset += fread(input + offset, 1, 1024, fp)) < *file_size);
    input[*file_size] = 0;

    /* Fill the file lines */
    line_n = 0;
    size_t line_s = *file_size >> 6 > 32 ? *file_size >> 6 : 32;
    file_lines = malloc(sizeof(char*) * line_s);

    char* end = input;
    while (*end)
    {
        if (line_n + 1 >= line_s)
        {
            line_s *= 2;
            file_lines = realloc(file_lines, sizeof(char*) * line_s);
        }

        file_lines[line_n++] = *end == '\n' ? ++end : end;
        while (*end && *end != '\n') end++;
    }

    return input;
}

void emit_warning(const TokenPosition* p, const char* format, ...)
{
    put_position(p, "Warning");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

int has_errors()
{
    return error_counter;
}

void emit_error(const TokenPosition* p, const char* format, ...)
{
    put_position(p, "Error");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    error_counter++;
}

void lexing_error_cb(const char* input,
                     const TokenPosition* position,
                     const char* lexer_state)
{
    (void) lexing_error_cb;
    (void) input;

    emit_error(position, "[state %s] Unmatched token near", lexer_state);
}

void parsing_error_cb(const char* const* token_names,
                      const TokenPosition* position,
                      uint32_t last_token,
                      uint32_t current_token,
                      const uint32_t expected_tokens[],
                      uint32_t expected_tokens_n)
{
    static char error_string[1024];
    error_string[0] = 0;

    strcat(error_string, "Unexpected token %s after %s\nExpected one of: ");
    for (int i = 0; i < expected_tokens_n; i++)
    {
        if (i > 0)
        {
            strcat(error_string, ", ");
        }

        strcat(error_string, token_names[expected_tokens[i]]);
    }

    emit_error(position, error_string,
               token_names[current_token],
               token_names[last_token]);
}

int main(int argc, const char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s [INPUT_FILE] [OUTPUT_FILE].cc? [OUTPUT_FILE].h\n", argv[0]);
        return 1;
    }

    char* input;
    FILE* fp = fopen(argv[1], "r");
    path = argv[1];
    if (!fp)
    {
        fprintf(stderr, "Failed to open '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    size_t file_size;
    input = read_file(fp, &file_size);
    fclose(fp);
    fp = NULL;

    cc_init();
    void* buf = cc_allocate_buffers();
    struct File* f = cc_parse_len(buf, input, file_size);

    cc_free();
    cc_free_buffers(buf);

    if (!f)
    {
        fprintf(stderr, "Failed to parse file '%s'\n", argv[1]);
        free(file_lines);
        free(input);
        return 1;
    }

    static char full_path[PATH_MAX];
    const char* full_file_path = realpath(argv[1], full_path);

    int error = codegen_write(full_file_path, f, argv[2], argv[3]);
    free(file_lines);
    free(input);
    file_free(f);

    return error;
}
