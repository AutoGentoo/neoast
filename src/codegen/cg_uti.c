#include <string.h>
#include <stdlib.h>
#include "cg_uti.h"


void put_enum(int start, int n, const char* const* names, FILE* fp)
{
    if (!n)
    {
        fputs("enum {};\n\n", fp);
        return;
    }

    fputs("enum\n{\n", fp);
    fprintf(fp, "    %s = %d,\n", names[0], start++);
    for (int i = 1; i < n; i++)
    {
        fprintf(fp, "    %s, // %d 0x%03X", names[i], start, start);
        if (strncmp(names[i], "ASCII_CHAR_0x", 13) == 0)
        {
            uint8_t c = strtoul(&names[i][13], NULL, 16);
            fprintf(fp, " '%c'\n", c);
        }
        else
        {
            fputc('\n', fp);
        }
        start++;
    }
    fputs("};\n\n", fp);
}

const char* check_grammar_arg_skip(const char* start, const char* search)
{
    // Assume we are not in comment or string
    int current_red_zone = -1;
    static const struct
    {
        const char* start_str;
        const char* end_str;
        uint32_t len[2];
    } red_zone_desc[] = {
            {"//", "\n", {2, 1}},
            {"/*", "*/", {2, 2}},
            {"\"", "\"", {1, 1}}
    };

    for (; (start < search || current_red_zone != -1) && *start; start++)
    {
        if (*start == '\\') // escape next character
            continue;

        for (int i = 0; i < sizeof(red_zone_desc) / sizeof(red_zone_desc[0]); i++)
        {
            if (current_red_zone == -1)
            {
                // Check for the start of a red zone
                if (strncmp(start, red_zone_desc[i].start_str, red_zone_desc[i].len[0]) == 0)
                {
                    current_red_zone = i;
                    start += red_zone_desc[i].len[0] - 1;
                    break;
                }
            } else
            {
                // Check for the end of a red zone
                if (strncmp(start, red_zone_desc[i].end_str, red_zone_desc[i].len[1]) == 0)
                {
                    current_red_zone = -1;
                    start += red_zone_desc[i].len[1] - 1;
                    break;
                }
            }
        }
    }

    return start;
}

int get_named_token(const char* token_name, const char* const* tokens, uint32_t token_n)
{
    for (int i = 0; i < token_n; i++)
    {
        if (strcmp(tokens[i], token_name) == 0)
            return i;
    }

    return -1;
}


int codegen_index(const char* const* array, const char* to_find, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (strcmp(array[i], to_find) == 0)
        {
            return i;
        }
    }

    return -1;
}

int codegen_parse_bool(const KeyVal* self)
{
    if (strcmp(self->value, "TRUE") == 0
        || strcmp(self->value, "true") == 0
        || strcmp(self->value, "True") == 0
        || strcmp(self->value, "1") == 0)
        return 1;
    else if (strcmp(self->value, "FALSE") == 0
             || strcmp(self->value, "false") == 0
             || strcmp(self->value, "False") == 0
             || strcmp(self->value, "0") == 0)
        return 0;

    emit_warning(&self->position,
                 "Unable to parse boolean value '%s', assuming FALSE",
                 self->value);
    return 0;
}

static void codegen_handle_option(
        struct Options* self,
        const KeyVal* option)
{
    if (strcmp(option->key, "parser_type") == 0)
    {
        if (strcmp(option->value, "LALR(1)") == 0)
        {
            self->parser_type = LALR_1;
        }
        else if (strcmp(option->value, "CLR(1)") == 0)
        {
            self->parser_type = CLR_1;
        }
        else
        {
            emit_error(&option->position, "Invalid parser type, support types: 'LALR(1)', 'CLR(1)'");
        }
    }
    else if (strcmp(option->key, "debug_table") == 0)
    {
        self->debug_table = codegen_parse_bool(option);
    }
    else if (strcmp(option->key, "track_position") == 0)
    {
        self->lexer_opts |= codegen_parse_bool(option) ? LEXER_OPT_TOKEN_POS : 0;
    }
    else if (strcmp(option->key, "track_position_type") == 0)
    {
        self->track_position_type = option->value;
    }
    else if (strcmp(option->key, "debug_ids") == 0)
    {
        self->debug_ids = option->value;
    }
    else if (strcmp(option->key, "prefix") == 0)
    {
        self->prefix = option->value;
    }
    else if (strcmp(option->key, "max_lex_tokens") == 0)
    {
        self->max_lex_tokens = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "max_token_len") == 0)
    {
        self->max_token_len = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "max_lex_state_depth") == 0)
    {
        self->max_lex_state_depth = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "parsing_stack_size") == 0)
    {
        self->parsing_stack_n = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "parsing_error_cb") == 0)
    {
        self->syntax_error_cb = option->value;
    }
    else if (strcmp(option->key, "lexing_error_cb") == 0)
    {
        self->lexing_error_cb = option->value;
    }
    else if (strcmp(option->key, "lex_match_longest") == 0)
    {
        self->lexer_opts |= codegen_parse_bool(option) ? LEXER_OPT_LONGEST_MATCH : 0;
    }
    else if (strcmp(option->key, "input_types") == 0)
    {
    }
    else
    {
        emit_error(&option->position, "Unsupported option, ignoring");
    }
}
