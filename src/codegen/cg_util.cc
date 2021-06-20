#include <vector>
#include <reflex/pattern.h>
#include <reflex/matcher.h>
#include "cg_util.h"

std::vector<std::pair<int, int>>
mark_redzones(const std::string& code)
{
    // Find all the regions where comments and string exist
    reflex::Pattern redzone_pattern(
            "(?mx)"
            "(\\/\\/[^\n]*\n)|"                 // Line comments
            "(\\/\\*(?:\\*(?!\\/)|[^*])*\\*\\/)|" // Block comments
            "(\"(?:[^\"\\\\]|\\\\[\\s\\S])*\")");   // String literals

    reflex::Matcher m(redzone_pattern, code);
    std::vector<std::pair<int, int>> out;

    for (auto& match : m.find)
    {
        out.emplace_back(match.first(), match.last());
    }

    return out;
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


void Options::handle(const KeyVal* option)
{
    if (strcmp(option->key, "parser_type") == 0)
    {
        if (strcmp(option->value, "LALR(1)") == 0)
        {
            parser_type = LALR_1;
        }
        else if (strcmp(option->value, "CLR(1)") == 0)
        {
            parser_type = CLR_1;
        }
        else
        {
            emit_error(&option->position, "Invalid parser type, support types: 'LALR(1)', 'CLR(1)'");
        }
    }
    else if (strcmp(option->key, "debug_table") == 0)
    {
        debug_table = codegen_parse_bool(option);
    }
    else if (strcmp(option->key, "track_position") == 0)
    {
        lexer_opts =
                static_cast<lexer_option_t>(
                        lexer_opts |
                        codegen_parse_bool(option) ? static_cast<int>(LEXER_OPT_TOKEN_POS) : 0);
    }
    else if (strcmp(option->key, "track_position_type") == 0)
    {
        track_position_type = option->value;
    }
    else if (strcmp(option->key, "debug_ids") == 0)
    {
        debug_ids = option->value;
    }
    else if (strcmp(option->key, "prefix") == 0)
    {
        prefix = option->value;
    }
    else if (strcmp(option->key, "parsing_stack_size") == 0)
    {
        parsing_stack_n = strtoul(option->value, nullptr, 0);
    }
    else if (strcmp(option->key, "parsing_error_cb") == 0)
    {
        syntax_error_cb = option->value;
    }
    else if (strcmp(option->key, "lexing_error_cb") == 0)
    {
        lexing_error_cb = option->value;
    }
    else if (strcmp(option->key, "lex_match_longest") == 0)
    {
        lexer_opts =
                static_cast<lexer_option_t>(
                        lexer_opts |
                        codegen_parse_bool(option) ? static_cast<int>(LEXER_OPT_LONGEST_MATCH) : 0);
    }
    else if (strcmp(option->key, "input_types") == 0)
    {
    }
    else if (strcmp(option->key, "lexer_file") == 0)
    {
        lexer_file = option->value;
    }
    else
    {
        emit_error(&option->position, "Unsupported option, ignoring");
    }
}
