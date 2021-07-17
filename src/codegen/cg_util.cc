#include <vector>
#include <reflex/pattern.h>
#include <reflex/matcher.h>
#include "cg_util.h"
#include "codegen_priv.h"

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

bool codegen_parse_bool(const KeyVal* self)
{
    if (strcmp(self->value, "TRUE") == 0
        || strcmp(self->value, "true") == 0
        || strcmp(self->value, "True") == 0
        || strcmp(self->value, "1") == 0)
        return true;
    else if (strcmp(self->value, "FALSE") == 0
             || strcmp(self->value, "false") == 0
             || strcmp(self->value, "False") == 0
             || strcmp(self->value, "0") == 0)
        return false;

    emit_warning(&self->position,
                 "Unable to parse boolean value '%s', assuming FALSE",
                 self->value);
    return false;
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
                        (codegen_parse_bool(option) ? static_cast<int>(LEXER_OPT_TOKEN_POS) : 0));
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
                        (codegen_parse_bool(option) ? static_cast<int>(LEXER_OPT_LONGEST_MATCH) : 0));
    }
    else if (strcmp(option->key, "input_types") == 0)
    {
        // TODO(tumbar) What is this?
    }
    else if (strcmp(option->key, "lexer_file") == 0)
    {
        lexer_file = option->value;
    }
    else if (strcmp(option->key, "no_warn_builtin") == 0)
    {
        no_warn_builtin = codegen_parse_bool(option);
    }
    else
    {
        emit_error(&option->position, "Unsupported option, ignoring");
    }
}


std::string Code::get_simple() const
{
    std::ostringstream ss;
    if (line && !file.empty())
    {
        ss << "#line "
           << line
           << " \"" << file.c_str() << "\"\n";
    }
    ss << code;
    return ss.str();
}

std::string Code::get_complex(
        const Options& options,
        const std::vector<std::string>& argument_replace,
        const std::string& zero_arg,
        const std::string& non_zero_arg,
        bool is_union) const
{
    std::ostringstream os;
    if (line && !file.empty())
    {
        os << "#line "
           << line
           << " \"" << file.c_str() << "\"\n";
    }

    auto redzones = mark_redzones(code);

    reflex::Pattern argument_pattern("[$](?:[$]|p?[0-9]+)");
    reflex::Matcher m(argument_pattern, code);

    size_t last_position = 0;
    for (auto& match : m.find)
    {
        if (m.first() > last_position)
        {
            os << code.substr(last_position, m.first() - last_position);
        }

        last_position = m.last();
        if (in_redzone(redzones, match))
        {
            os << m.text();
            continue;
        }

        TokenPosition match_pos = {line, static_cast<uint32_t>(col_start + match.first())};
        if (match.text()[1] == '$')
        {
            if (is_union)
            {
                os << zero_arg << "->" << argument_replace[0];
            }
            else
            {
                os << zero_arg << "->value." << argument_replace[0];
            }
        }
        else if (match.text()[1] == 'p')
        {
            if (is_union)
            {
                emit_error(&match_pos, "Illegal yyval position $p in lexer rule");
                continue;
            }

            size_t idx;
            idx = std::stol(match.text() + 2, nullptr, 10);

            if (!(options.lexer_opts & LEXER_OPT_TOKEN_POS))
            {
                emit_error(&match_pos, "Attempting to get token position without track_position=\"TRUE\"");
                break;
            }
            else if (idx >= argument_replace.size())
            {
                emit_error(&match_pos, "Position index out of range, function only has '%d' arguments",
                           argument_replace.size());
                continue;
            }
            else if (idx == 0)
            {
                emit_error(&match_pos, "Invalid position argument index '0' use $p1");
                continue;
            }

            if (!options.track_position_type.empty())
            {
                os << variadic_string("((const %s*)&%s[%d].position)",
                                      options.track_position_type.c_str(),
                                      non_zero_arg.c_str(),
                                      idx - 1);
            }
            else
            {
                os << variadic_string("((const TokenPosition*)&args[%d].position)", idx - 1);
            }
        }
        else
        {
            if (is_union)
            {
                emit_error(&match_pos, "Illegal non destination $N in lexer rule");
                continue;
            }

            size_t idx;
            try
            {
                idx = std::stol(match.text() + 1, nullptr, 10);
            }
            catch (const std::exception& e)
            {
                emit_error(&match_pos, "Invalid argument, excepted integer: %s", e.what());
                continue;
            }

            if (idx >= argument_replace.size())
            {
                emit_error(&match_pos, "Argument index out of range, function only has '%d' arguments",
                           argument_replace.size());
                continue;
            }
            else if (idx == 0)
            {
                emit_error(&match_pos, "Invalid argument index '0', use '$$' for destination");
                continue;
            }
            else if (argument_replace[idx].empty())
            {
                emit_error(&match_pos, "Argument '%d' does not have type", idx);
                continue;
            }

            os << non_zero_arg << "[" << idx - 1 << "].value." << argument_replace[idx];
        }
    }

    os << code.substr(last_position) << "\n";
    return os.str();
}