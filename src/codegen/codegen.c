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


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <lexer.h>
#include "stdio.h"
#include "codegen.h"
#include "regex.h"
#include <parsergen/canonical_collection.h>
#include <util/util.h>
#include <stddef.h>

#define CODEGEN_STRUCT "NeoastValue"
#define CODEGEN_UNION "NeoastUnion"

struct Options {
    // Should we dump the table
    int debug_table;
    const char* track_position_type;
    const char* debug_ids;
    const char* prefix;
    const char* lexing_error_cb;
    const char* syntax_error_cb;
    parser_t parser_type; // LALR(1) or CLR(1)

    unsigned long max_lex_tokens;
    unsigned long max_token_len;
    unsigned long max_lex_state_depth;
    unsigned long parsing_stack_n;

    lexer_option_t lexer_opts;
};

static inline
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

static inline
void put_lexer_rule_action(
        const char* grammar_file_path,
        struct LexerRuleProto* self,
        const char* state_name,
        uint32_t regex_i,
        FILE* fp)
{
    fprintf(fp, "static int32_t\nll_rule_%s_%02d(const char* yytext, "
                CODEGEN_UNION"* yyval, "
                "unsigned int len, "
                "ParsingStack* lex_state, "
                "TokenPosition* position"
                ")\n{\n"
                "    (void) yytext;\n"
                "    (void) yyval;\n"
                "    (void) len;\n"
                "    (void) lex_state;\n"
                "    (void) position;\n"
                "    {", state_name, regex_i);

    // Put a line directive to tell the compiler
    // where to original code resides
    if (self->position.line && grammar_file_path)
    {
        fprintf(fp, "\n#line %d \"%s\"\n", self->position.line, grammar_file_path);
    }

    fputs(self->function, fp);
    fputs("}\n    return -1;\n}\n\n", fp);
}

static inline
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

static inline
int get_named_token(const char* token_name, const char* const* tokens, uint32_t token_n)
{
    for (int i = 0; i < token_n; i++)
    {
        if (strcmp(tokens[i], token_name) == 0)
            return i;
    }

    return -1;
}

static inline
const char* put_grammar_rule_arg(
        const char* search,
        struct GrammarRuleProto* parent,
        struct GrammarRuleSingleProto* self,
        const char** tokens,
        const struct KeyVal** typed_tokens,
        const struct Options* options,
        uint32_t token_n,
        FILE* fp)
{
    int search_offset = 1;
    if (search[1] == '$')
    {
        // Return value
        int expression_token = get_named_token(parent->name, tokens, token_n);
        if (expression_token == -1)
        {
            emit_error(&self->position, "Rule not defined", parent->name);
            return NULL;
        }

        assert(typed_tokens[expression_token]);

        fprintf(fp, "dest->%s", typed_tokens[expression_token]->key);
        return search + 2;
    }
    else if (search[1] == 'p')
    {
        // Positional token
        search_offset = 2;
    }

    char* out;
    uint32_t arg_num = strtoul(search + search_offset, &out, 10);
    if (arg_num == 0)
    {
        emit_error(&self->position, "Invalid argument index '0', use '$$' for destination");
        return NULL;
    }

    struct Token* tok = self->tokens;
    int i;
    for (i = 0; i < arg_num - 1 && tok; i++)
    {
        tok = tok->next;
    }

    if (!tok)
    {
        emit_error(&self->position,
                   "Invalid argument index '%d', expression only has '%d' arguments",
                   arg_num, i);
        return NULL;
    }

    if (search[1] == 'p')
    {
        if (!(options->lexer_opts & LEXER_OPT_TOKEN_POS))
        {
            emit_error(&self->position, "Attempting to get token position without track_position=\"TRUE\"");
            return NULL;
        }

        if (options->track_position_type)
        {
            fprintf(fp, "((const %s*)&args[%d].position)", options->track_position_type, arg_num - 1);
        }
        else
        {
            fprintf(fp, "((const TokenPosition*)&args[%d].position)", arg_num - 1);
        }
    }
    else
    {
        int token_id = get_named_token(tok->name, tokens, token_n);
        if (token_id < 0)
        {
            emit_error(&tok->position, "Token has not been defined");
            return NULL;
        }

        if (!typed_tokens[token_id])
        {
            emit_error(&tok->position, "Token token without a type cannot be used in action");
            return NULL;
        }

        fprintf(fp, "args[%d].value.%s", arg_num - 1, typed_tokens[token_id]->key);
    }

    return out;
}

static inline
void put_grammar_rule_action(
        const char* grammar_file_path,
        struct GrammarRuleProto* parent,
        struct GrammarRuleSingleProto* self,
        const char** tokens,
        const struct KeyVal** typed_tokens,
        const struct Options* options,
        uint32_t token_n,
        uint32_t rule_n,
        FILE* fp)
{
    fprintf(fp, "static void\ngg_rule_r%02d("
                CODEGEN_UNION"* dest, "
                CODEGEN_STRUCT"* args)\n{\n"
                "    (void) dest;\n"
                "    (void) args;\n"
                "    {", rule_n);

    int stop = 0;

    // Put a line directive to tell the compiler
    // where to original code resides
    if (self->position.line && grammar_file_path)
    {
        // Return value
        int expression_token = get_named_token(parent->name, tokens, token_n);
        if (expression_token == -1)
        {
            emit_error(&self->position, "Rule not defined", parent->name);
            stop = 1;
        }

        assert(typed_tokens[expression_token]);

        fprintf(fp, "\n#line %d \"%s\"\n", self->position.line, grammar_file_path);

        if (!stop)
        {
            int space_count = self->position.col_start -
                              strlen(typed_tokens[expression_token]->key) - 4;
            for (int i = 0; i < space_count; i++)
            {
                fputc(' ', fp);
            }
        }
    }

    // Find all usages of $N or $$
    // Check if this usage should be ignored (comment or string)
    // Replace this usage with the appropriate C-value

    const char* search = NULL;
    const char* start = self->function;
    while ((search = strchr(start, '$')) && !stop)
    {
        if ((search[1] >= '0' && search[1] <= '9') || search[1] == '$' || search[1] == 'p')
        {
            // Check if this string is in a comment or string
            const char* finish_red = check_grammar_arg_skip(start, search);
            if (finish_red > search)
            {
                // Skip this '$'
                search = finish_red;
            } else
            {
                // Print the content until this point
                fwrite(start, 1, search - start, fp);

                // Print the argument
                start = put_grammar_rule_arg(search, parent, self, tokens, typed_tokens,
                                             options, token_n, fp);
                if (!start)
                {
                    break;
                }
                continue;
            }
        }

        // Not an argument
        fwrite(start, 1, search - start, fp);
        start = search;
    }

    if (start)
    {
        fputs(start, fp);
    }

    fputs("\n    }\n}\n\n", fp);
}

static inline
void put_destructor_action(const struct KeyVal* destructor, FILE* fp)
{
    fprintf(fp, "static void\n%s_destructor("
                CODEGEN_UNION"* self)\n{\n"
                "    {", destructor->key);

    const char* search = NULL;
    const char* start = destructor->value;

    while ((search = strchr(start, '$')))
    {
        if (search[1] == '$')
        {
            // Check if this string is in a comment or string
            const char* finish_red = check_grammar_arg_skip(start, search);
            if (finish_red > search)
            {
                // Skip this '$'
                search = finish_red;
            } else
            {
                // Print the content until this point
                fwrite(start, 1, search - start, fp);

                // Print the argument
                fprintf(fp, "self->%s", destructor->key);
                start = search + 2;
                continue;
            }
        }

        // Not an argument
        fwrite(start, 1, search - start, fp);
        start = search;
    }

    fputs(start, fp);
    fputs("}\n}\n\n", fp);
}

static inline
void put_lexer_rule_regex(LexerRule* self, FILE* fp)
{
    fputs("        ", fp);
    for (const char* iter = self->regex_raw; *iter; iter++)
    {
        fprintf(fp, "0x%02x, ", *iter);
    }

    // Null terminator
    fprintf(fp, "0x%02x,\n", 0);
}

static inline
uint32_t put_lexer_rule(LexerRule* self, const char* state_name, uint32_t offset, uint32_t rule_i, FILE* fp)
{
    if (self->expr)
    {
        fprintf(fp, "        {"
                    ".expr = (lexer_expr) ll_rule_%s_%02d, "
                    ".regex_raw = &ll_rules_state_%s_regex_table[%d],"
                    "}, // %s\n",
                state_name,  rule_i, state_name, offset, self->regex_raw);
    } else
    {
        // TODO Add support for quick token optimization in code gen
    }

    return strlen(self->regex_raw) + 1;
}

static inline
void put_lexer_state_rules(LexerRule* rules, uint32_t rules_n,
                           const char* state_name, FILE* fp)
{
    // First we need to build the regex table
    fprintf(fp, "static const char ll_rules_state_%s_regex_table[] = {\n", state_name);
    for (uint32_t i = 0; i < rules_n; i++)
    {
        put_lexer_rule_regex(&rules[i], fp);
    }
    fputs("};\n\n", fp);

    fprintf(fp, "static LexerRule ll_rules_state_%s[] = {\n", state_name);
    uint32_t offset = 0;
    for (int i = 0; i < rules_n; i++)
    {
        offset += put_lexer_rule(&rules[i], state_name, offset, i, fp);
    }

    fputs("};\n\n", fp);
}

static inline
void put_lexer_rule_count(const uint32_t* ll_rule_count, int lex_state_n, FILE* fp)
{
    fputs("static const uint32_t lexer_rule_n[] = {", fp);
    for (int i = 0; i < lex_state_n; i++)
    {
        if (i + 1 >= lex_state_n)
        {
            fprintf(fp, "%d", ll_rule_count[i]);
        }
        else
        {
            fprintf(fp, "%d, ", ll_rule_count[i]);
        }
    }
    fputs("};\n", fp);
}

static inline
void put_lexer_states(const char* const* states_names, int states, FILE* fp)
{
    fputs("static LexerRule* __neoast_lexer_rules[] = {\n", fp);
    for (int i = 0; i < states; i++)
    {
        fprintf(fp, "        ll_rules_state_%s,\n", states_names[i]);
    }

    fputs("};\n\n", fp);
}

static inline
void put_grammar_table_and_rules(
        struct GrammarRuleProto* rule_symbols,
        GrammarRule* rules,
        const char* const* tokens,
        uint32_t rule_n,
        FILE* fp)
{
    // First put the grammar table
    fputs("static const\nunsigned int grammar_token_table[] = {\n", fp);
    uint32_t grammar_i = 0;
    for (struct GrammarRuleProto* rule_iter = rule_symbols;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        fprintf(fp, "        /* %s */\n", rule_iter->name);
        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            if (grammar_i == 0)
            {
                grammar_i++;
                fputs("        /* ACCEPT */ ", fp);
            }
            else
            {
                fprintf(fp,"        /* R%02d */ ", grammar_i++);
            }
            if (rule_single_iter->tokens)
            {
                for (struct Token* tok = rule_single_iter->tokens; tok; tok = tok->next)
                {
                    fprintf(fp, "%s,%c", tok->name, tok->next ? ' ' : '\n');
                }
            } else
            {
                fputs("// empty rule\n", fp);
            }
        }

        fputc('\n', fp);
    }
    fputs("};\n\n", fp);

    // Print the grammar rules
    uint32_t grammar_offset_i = 0;
    fputs("static const\nGrammarRule __neoast_grammar_rules[] = {\n", fp);
    for (int i = 0; i < rule_n; i++)
    {
        if (rules[i].expr)
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d], .expr=(parser_expr) gg_rule_r%02d},\n",
                    tokens[rules[i].token - NEOAST_ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i,
                    i);
        }
        else
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d]},\n",
                    tokens[rules[i].token - NEOAST_ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i);
        }

        grammar_offset_i += rules[i].tok_n;
    }
    fputs("};\n\n", fp);
}

static inline
void put_parsing_table(const uint32_t* parsing_table, CanonicalCollection* cc, FILE* fp)
{
    int i = 0;
    fputs("static const\nuint32_t GEN_parsing_table[] = {\n", fp);
    for (int state_i = 0; state_i < cc->state_n; state_i++)
    {
        fputs("        ", fp);
        for (int tok_i = 0; tok_i < cc->parser->token_n; tok_i++, i++)
        {
            fprintf(fp,"0x%08X,%c", parsing_table[i], tok_i + 1 >= cc->parser->token_n ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
}

static inline
void put_ascii_mappings(const uint32_t ascii_mappings[NEOAST_ASCII_MAX], FILE* fp)
{
    fputs("static const\nuint32_t __neoast_ascii_mappings[NEOAST_ASCII_MAX] = {\n", fp);
    int i = 0;
    for (int row = 0; i < NEOAST_ASCII_MAX; row++)
    {
        fputs("        ", fp);
        for (int col = 0; col < 6 && i < NEOAST_ASCII_MAX; col++, i++)
        {
            fprintf(fp, "0x%03X,%c", ascii_mappings[i], (col + 1 >= 6) ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
}

static inline
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

static int codegen_parse_bool(const struct KeyVal* self)
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
        const struct KeyVal* option)
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
    else
    {
        emit_error(&option->position, "Unsupported option, ignoring");
    }
}

int codegen_write(
        const char* grammar_file_path,
        const struct File* self,
        FILE* fp)
{
    // Codegen steps:
    //   1. Write the header
    //   2. Generate enums and union
    //   3. Dump lexer actions
    //   4. Dump parser actions
    //   5. Dump lexer rules structures (all states)
    //   6. Dump parser rules structure
    //   7. Generate table and write
    //   8. Generate entry point and buffer creation

    const struct KeyVal* _header = NULL;
    const struct KeyVal* _bottom = NULL;
    const struct KeyVal* _union = NULL;
    const struct KeyVal* _start = NULL;

    int action_n = 1, token_n = 1, // reserved eof
    typed_token_n = 0, precedence_n = 0,
            lex_state_n = 1;

    int destructor_s__ = 128;
    int destructor_i__ = 0;
    struct DestroyMe
    {
        void* ptr;
        void (* func)(void*);
    }* destructors__ = malloc(sizeof(struct DestroyMe) * destructor_s__);

#define DESTROY_ME(ptr_, func_) do { \
        if (destructor_i__ + 1 >= destructor_s__) {     \
            destructor_s__ <<= 1;                       \
            destructors__ = realloc(destructors__, sizeof(struct DestroyMe) * destructor_s__); \
            destructors__[0].ptr = destructors__;       \
        }                                               \
        destructors__[destructor_i__].ptr = (ptr_);     \
        destructors__[destructor_i__].func = (func_); \
        destructor_i__++;                     \
    } while (0)

#define DESTROY_ALL do { while (destructor_i__--) {    \
        destructors__[destructor_i__].func(destructors__[destructor_i__].ptr); \
    } } while (0)

#define EXIT_IF_ERRORS do { if (has_errors()) { DESTROY_ALL; return -1; } } while (0)

    DESTROY_ME(destructors__, free);

    struct Options options = {
            .debug_table = 0,
            .parser_type = LALR_1,
            .debug_ids = NULL,
            .track_position_type = NULL,
            .lexing_error_cb = NULL,
            .syntax_error_cb = NULL,
            .prefix = "neoast",
            .max_token_len = 1024,
            .max_lex_tokens = 1024,
            .max_lex_state_depth = 16,
            .parsing_stack_n = 1024,
    };

    MacroEngine* m_engine = macro_engine_init();
    DESTROY_ME(m_engine, (void (*) (void*))macro_engine_free);

    // Iterate a single time though the header data
    // Count all of the different header option types
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_TOP:
                if (_header)
                {
                    emit_error(&iter->position, "cannot have two %%top definitions");
                    return -1;
                }

                _header = iter;
                break;
            case KEY_VAL_BOTTOM:
                if (_bottom)
                {
                    emit_error(&iter->position, "cannot have two %%bottom definitions");
                    return -1;
                }

                _bottom = iter;
                break;
            case KEY_VAL_UNION:
                if (_union)
                {
                    emit_error(&iter->position, "cannot have two %%union definitions");
                    return -1;
                }

                _union = iter;
                break;
            case KEY_VAL_START:
                if (_start)
                {
                    emit_error(&iter->position, "cannot have two %%start definitions");
                }

                _start = iter;
                break;
            case KEY_VAL_TOKEN:
            case KEY_VAL_TOKEN_ASCII:
                action_n++;
                token_n++;
                break;
            case KEY_VAL_TOKEN_TYPE:
                action_n++;
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_TYPE:
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_OPTION:
                codegen_handle_option(&options, iter);
                break;
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                precedence_n++;
                break;
            case KEY_VAL_MACRO:
                macro_engine_register(m_engine, iter->key, iter->value);
                break;
            case KEY_VAL_DESTRUCTOR:
                break;
        }
    }

    if (!_start || !_union)
    {
        TokenPosition pos = {1, 0};
        emit_error(&pos, "%%start and %%union are required");
        return -1;
    }

    if (!action_n || token_n == action_n)
    {
        TokenPosition pos = {1, 0};
        emit_error(&pos, "no lexer or parser tokens are defined");
        return -1;
    }

    EXIT_IF_ERRORS;

    token_n++; // augment token

    // Now that we know the count of every type, we can initialize them
    int action_i = 0, grammar_i = action_n,
            typed_token_i = 0, lex_state_i = 1;
    const char** tokens = calloc(token_n, sizeof(char*));
    const struct KeyVal** typed_tokens = calloc(token_n, sizeof(struct KeyVal*));
    const struct KeyVal** destructors = calloc(token_n, sizeof(struct KeyVal*));
    DESTROY_ME(tokens, free);
    DESTROY_ME(typed_tokens, free);
    DESTROY_ME(destructors, free);

    uint8_t* precedence_table = calloc(token_n, sizeof(uint8_t));
    DESTROY_ME(precedence_table, free);

    uint32_t ascii_mappings[NEOAST_ASCII_MAX] = {0};

    tokens[action_i++] = "EOF";
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_TOKEN:
                if (codegen_index(tokens, iter->value, action_i) != -1)
                {
                    emit_error(&iter->position, "Duplicate token declaration");
                }
                tokens[action_i++] = iter->value;
                break;
            case KEY_VAL_TOKEN_TYPE:
                if (codegen_index(tokens, iter->value, action_i) != -1)
                {
                    emit_error(&iter->position, "Duplicate token declaration");
                }
                typed_tokens[action_i] = iter;
                tokens[action_i++] = iter->value;
                typed_token_i++;
                break;
            case KEY_VAL_TOKEN_ASCII:
                if (codegen_index(tokens, iter->value, action_i) != -1)
                {
                    emit_error(&iter->position, "Duplicate token declaration");
                }
                ascii_mappings[get_ascii_from_name(iter->value)] = action_i + NEOAST_ASCII_MAX;
                tokens[action_i++] = iter->value;
                break;
            case KEY_VAL_TYPE:
                if (codegen_index(tokens + action_n, iter->value, grammar_i - action_n) != -1)
                {
                    emit_error(&iter->position, "Duplicate type declaration");
                }
                typed_tokens[grammar_i] = iter;
                tokens[grammar_i++] = iter->value;
                typed_token_i++;
                break;
            default:
                break;
        }
    }

    EXIT_IF_ERRORS;

    tokens[grammar_i++] = "TOK_AUGMENT";


    assert(typed_token_i == typed_token_n);
    assert(action_i == action_n);
    assert(grammar_i == token_n);
    assert(lex_state_i == lex_state_n);

    // Write the header information
    fputs("#define NEOAST_PARSER_CODEGEN___C\n"
          "#include <neoast.h>\n"
          "#include <string.h>\n", fp);

    if (_header)
    {
        fputs(_header->value, fp);
        fputc('\n', fp);
    }

    fprintf(fp, "typedef union {%s} " CODEGEN_UNION ";\n", (char*)_union->value);
    if (options.lexer_opts & LEXER_OPT_TOKEN_POS)
    {
        fprintf(fp, "typedef struct {" CODEGEN_UNION " value; TokenPosition position;} " CODEGEN_STRUCT ";\n\n");
    }
    else
    {
        fprintf(fp, "typedef struct {" CODEGEN_UNION " value;}" CODEGEN_STRUCT ";\n\n");
    }

    fputs("// Tokens\n", fp);
    // We can cast this because the first field in Token is name
    put_enum(NEOAST_ASCII_MAX + 1, token_n - 1, (const char**)tokens + 1, fp);

    // Map a lexer state id to each state
    int lexer_state_s = 32;
    const char** lexer_states = malloc(sizeof(char*) * lexer_state_s);
    uint32_t* ll_rule_count = malloc(sizeof(uint32_t) * lexer_state_s);
    DESTROY_ME(lexer_states, free);
    DESTROY_ME(ll_rule_count, free);

    lexer_states[0] = "LEX_STATE_DEFAULT";
    ll_rule_count[0] = 0;

    // Dump all lexer rule actions and count number of rules in each state
    for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        if (lex_state_n + 1 >= lexer_state_s)
        {
            lexer_state_s *= 2;
            lexer_states = realloc(lexer_states, sizeof(char*) * lexer_state_s);
            ll_rule_count = realloc(ll_rule_count, sizeof(uint32_t) * lexer_state_s);
        }

        if (iter->lexer_state)
        {
            assert(!iter->function);
            assert(!iter->regex);
            assert(iter->state_rules);
            int state_id = lex_state_n++;
            lexer_states[state_id] = iter->lexer_state;
            ll_rule_count[state_id] = 0;

            // Count the number of rules in this state
            for (struct LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
            {
                ll_rule_count[state_id]++;
            }
        }
        else
        {
            ll_rule_count[0]++;
        }
    }

    // Dump the destructor actions
    // Generate the precedence table
    // Generate the destructor table
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        int token_id;
        int install_count;
        switch (iter->type)
        {
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                token_id = codegen_index(tokens, iter->value, token_n);
                if (token_id >= action_n)
                {
                    emit_warning(&iter->position, "Cannot use %%type token in precedence");
                    continue;
                }
                else if (token_id < 0)
                {
                    emit_error(&iter->position, "Invalid token name");
                    continue;
                }

                precedence_table[token_id] = iter->type == KEY_VAL_LEFT ? PRECEDENCE_LEFT : PRECEDENCE_RIGHT;
                break;
            case KEY_VAL_DESTRUCTOR:
                // Install destructor in every token with this type
                install_count = 0;
                for (int i = 0; i < token_n; i++)
                {
                    if (typed_tokens[i] && strcmp(typed_tokens[i]->key, iter->key) == 0)
                    {
                        destructors[i] = iter;
                        install_count++;
                    }
                }

                if (!install_count)
                {
                    emit_error(&iter->position, "No token matches type '%s'", iter->key);
                }

                // Dump this destructor action
                put_destructor_action(iter, fp);
            default:
                break;
        }
    }

    EXIT_IF_ERRORS;

    fputs("// Lexer states\n"
          "#ifndef NEOAST_EXTERNAL_INCLUDE\n"
          "#define NEOAST_EXTERNAL_INCLUDE\n", fp);
    // Yes this is a safe cast
    put_enum(0, lex_state_n, lexer_states, fp);

    // Dump the lexer rules
    LexerRule** ll_rules = malloc(sizeof(LexerRule*) * lex_state_n);
    DESTROY_ME(ll_rules, free);

    int iterator = 0;
    ll_rules[0] = malloc(sizeof(LexerRule) * ll_rule_count[0]);
    DESTROY_ME(ll_rules[0], free);

    for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        if (iter->lexer_state)
        {
            int state_iterator = 0;
            int state_id = codegen_index(lexer_states, iter->lexer_state, lex_state_n);
            assert(state_id > 0);

            ll_rules[state_id] = malloc(sizeof(LexerRule) * ll_rule_count[state_id]);
            DESTROY_ME(ll_rules[state_id], free);
            LexerRule* current = ll_rules[state_id];

            for (struct LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
            {
                assert(state_iterator < ll_rule_count[0]);
                assert(iter_s->regex);
                assert(iter_s->function);
                assert(!iter_s->lexer_state);
                assert(!iter_s->state_rules);
                put_lexer_rule_action(grammar_file_path, iter_s, lexer_states[state_id], state_iterator, fp);
                LexerRule* ll_rule = &ll_rules[state_id][state_iterator++];
                ll_rule->expr = (lexer_expr) iter_s;
                char* expanded_regex = regex_expand(m_engine, iter_s->regex);
                DESTROY_ME(expanded_regex, free);
                ll_rule->regex_raw = expanded_regex;
                ll_rule->tok = 0;
                if (!regex_verify(m_engine, ll_rule->regex_raw))
                {
                    emit_error(&iter->position, "Failed to compile regular expression");
                }
            }

            assert(state_iterator == ll_rule_count[state_id]);
            assert(lexer_states[state_id]);
            put_lexer_state_rules(current, ll_rule_count[state_id], lexer_states[state_id], fp);
        }
        else
        {
            assert(iterator < ll_rule_count[0]);
            assert(iter->regex);
            assert(iter->function);
            assert(!iter->lexer_state);
            assert(!iter->state_rules);
            put_lexer_rule_action(grammar_file_path, iter, lexer_states[0], iterator, fp);
            LexerRule* ll_rule = &ll_rules[0][iterator++];
            ll_rule->expr = (lexer_expr) iter;
            char* expanded_regex = regex_expand(m_engine, iter->regex);
            DESTROY_ME(expanded_regex, free);
            ll_rule->regex_raw = expanded_regex;
            ll_rule->tok = 0;
            if (!regex_verify(m_engine, ll_rule->regex_raw))
            {
                emit_error(&iter->position, "Failed to compile regular expression");
            }
            assert(lexer_states[0]);
        }
    }

    assert(iterator == ll_rule_count[0]);
    put_lexer_state_rules(ll_rules[0], ll_rule_count[0], lexer_states[0], fp);

    put_lexer_rule_count(ll_rule_count, lex_state_n, fp);
    put_lexer_states(lexer_states, lex_state_n, fp);

    // Dump the destructor rules
    fputs("static const\nparser_destructor __neoast_token_destructors[] = {\n", fp);
    for (int i = 0; i < token_n; i++)
    {
        if (!destructors[i])
        {
            fprintf(fp, "        NULL, // %s\n", tokens[i]);
        }
        else
        {
            fprintf(fp, "        (parser_destructor) %s_destructor, // %s\n", destructors[i]->key, tokens[i]);
        }
    }
    fputs("};\n\n", fp);

    // Dump the grammar rule actions
    uint32_t grammar_n = 0;
    uint32_t grammar_tok_n = 0;
    for (struct GrammarRuleProto* rule_iter = self->grammar_rules;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        // Make sure this is a %type not a %token
        int type_id = get_named_token(rule_iter->name, tokens, token_n);
        if (type_id < action_n)
        {
            emit_error(&rule_iter->position, "Grammar rule defined for %%token - did you mean %%type?");
            continue;
        }

        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            put_grammar_rule_action(grammar_file_path,
                                    rule_iter, rule_single_iter, tokens, typed_tokens,
                                    &options,
                                    token_n, grammar_n + 1, fp);
            grammar_n++;

            // Find the number of tokens in this rule
            for (struct Token* tok_iter = rule_single_iter->tokens; tok_iter; tok_iter = tok_iter->next)
            {
                grammar_tok_n++;
            }
        }
    }

    EXIT_IF_ERRORS;

    // Add the augment rule
    struct Token augment_rule_tok = {
            .next = NULL,
            .name = _start->value
    };
    struct GrammarRuleSingleProto augment_rule = {
            .next = NULL,
            .tokens = &augment_rule_tok
    };
    struct GrammarRuleProto augment_rule_parent = {
            .next = self->grammar_rules,
            .rules = &augment_rule,
            .name = "TOK_AUGMENT"
    };

    struct GrammarRuleProto* all_rules = &augment_rule_parent;
    grammar_n++;
    grammar_tok_n++;

    // Dump the grammar rules as well as augment rule
    GrammarRule* gg_rules = malloc(sizeof(GrammarRule) * grammar_n);
    int32_t* grammar_table = malloc(sizeof(uint32_t) * grammar_tok_n);

    DESTROY_ME(gg_rules, free);
    DESTROY_ME(grammar_table, free);

    uint32_t grammar_tok_offset_c = 0;
    grammar_i = 0;

    for (struct GrammarRuleProto* rule_iter = all_rules;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        int rule_tok = codegen_index(tokens, rule_iter->name, token_n);
        if (rule_tok == -1)
        {
            emit_error(&rule_iter->position, "Could not find token for rule '%s'\n", rule_iter->name);
            continue;
        }

        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            uint32_t grammar_table_offset = grammar_tok_offset_c;
            uint32_t gg_tok_n = 0;
            // Add the tokens to the token tables
            for (struct Token* tok_iter = rule_single_iter->tokens; tok_iter; tok_iter = tok_iter->next)
            {
                grammar_table[grammar_tok_offset_c] = codegen_index(tokens, tok_iter->name, token_n) + NEOAST_ASCII_MAX;
                if (grammar_table[grammar_tok_offset_c] < NEOAST_ASCII_MAX)
                {
                    emit_error(&tok_iter->position, "Undeclared token '%s'", tok_iter->name);
                    continue;
                }
                grammar_tok_offset_c++;
                gg_tok_n++;
            }

            // Construct the rule
            GrammarRule* gg_current = &gg_rules[grammar_i++];
            gg_current->expr = (parser_expr) rule_single_iter->function;
            gg_current->token = rule_tok + NEOAST_ASCII_MAX;
            gg_current->grammar = (uint32_t*) &grammar_table[grammar_table_offset];

            gg_current->tok_n = gg_tok_n;
        }
    }

    EXIT_IF_ERRORS;
    assert(grammar_tok_offset_c == grammar_tok_n);
    put_grammar_table_and_rules(all_rules, gg_rules, tokens, grammar_n, fp);

    // Initialize the parser
    GrammarParser parser = {
            .token_n = token_n - 1,
            .token_names = tokens,
            .lex_state_n = lex_state_n,
            .action_token_n = action_n,
            .lex_n = ll_rule_count,
            .lexer_rules = ll_rules,
            .grammar_n = grammar_n,
            .grammar_rules = gg_rules,
            .ascii_mappings = ascii_mappings,
    };

    // Dump parser instantiation
    fputs("static const\nchar* __neoast_token_names[] = {\n", fp);
    for (int i = 0; i < token_n - 1; i++)
    {
        if (strncmp(tokens[i], "ASCII_CHAR_0x", 13) == 0)
        {
            fprintf(fp, "        \"%c\",\n", get_ascii_from_name(tokens[i]));
        }
        else
        {
            fprintf(fp, "        \"%s\",\n", tokens[i]);
        }
    }
    fputs("};\n\n", fp);

    put_ascii_mappings(ascii_mappings, fp);

    fprintf(fp, "static GrammarParser parser = {\n"
                "        .grammar_n = %d,\n"
                "        .lex_state_n = %d,\n"
                "        .lex_n = lexer_rule_n,\n"
                "        .ascii_mappings = __neoast_ascii_mappings,\n"
                "        .lexer_rules = __neoast_lexer_rules,\n"
                "        .grammar_rules = __neoast_grammar_rules,\n"
                "        .token_names = __neoast_token_names,\n"
                "        .destructors = __neoast_token_destructors,\n"
                "        .lexer_error = %s,\n"
                "        .parser_error = %s,\n"
                "        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,\n"
                "        .action_token_n = %d,\n"
                "        .lexer_opts = (lexer_option_t)%d,\n",
            grammar_n, lex_state_n,
            options.lexing_error_cb ? options.lexing_error_cb : "NULL",
            options.syntax_error_cb ? options.syntax_error_cb : "NULL",
            action_n, options.lexer_opts
    );
    fputs("};\n\n", fp);

    EXIT_IF_ERRORS;

    CanonicalCollection* cc = canonical_collection_init(&parser);
    canonical_collection_resolve(cc, options.parser_type);
    DESTROY_ME(cc, (void (*) (void*))canonical_collection_free);

    uint8_t error;
    uint32_t* parsing_table = canonical_collection_generate(cc, precedence_table, &error);
    DESTROY_ME(parsing_table, free);

    if (options.debug_table)
    {
        fprintf(fp, "// Token names:\n");
        char* fallback = malloc(token_n + 1);
        for (int i = 0; i < token_n - 1; i++)
        {
            if (i == 0)
            {
                fallback[i] = '$';
            }
            else if (i < action_n)
            {
                fallback[i] = (char)('a' + (char)i - 1);
            }
            else
            {
                fallback[i] = (char)('A' + (char)i - (action_n));
            }
        }

        if (!options.debug_ids) options.debug_ids = fallback;
        for (int i = 0; i < token_n - 1; i++)
        {
            if (strncmp(tokens[i], "ASCII_CHAR_0x", 13) == 0)
            {
                fprintf(fp, "//  %s => '%c' ('%c')\n",
                        tokens[i], options.debug_ids[i],
                        get_ascii_from_name(tokens[i]));
            }
            else
            {
                fprintf(fp, "//  %s => '%c'\n", tokens[i], options.debug_ids[i]);
            }
        }

        fputs("\n", fp);
        dump_table(parsing_table, cc, options.debug_ids, 0, fp, "//  ");
        free(fallback);
    }

    // Dump the actual parsing table
    put_parsing_table(parsing_table, cc, fp);

    // Dump the exported symbols
    fprintf(fp, "static int parser_initialized = 0;\n"
                "uint32_t %s_init()\n{\n"
                "    if (!parser_initialized)\n"
                "    {\n"
                "        uint32_t error = parser_init(&parser);\n"
                "        if (error)\n"
                "            return error;\n\n"
                "        parser_initialized = 1;\n"
                "    }\n"
                "    return 0;\n"
                "}\n\n",
                options.prefix);
    fprintf(fp, "void %s_free()\n"
                "{\n"
                "    if (parser_initialized)\n"
                "    {\n"
                "         parser_free(&parser);\n"
                "         parser_initialized = 0;\n"
                "    }\n"
                "}\n\n",
                options.prefix);
    if (options.lexer_opts & LEXER_OPT_TOKEN_POS)
    {
        fprintf(fp, "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, %lu, %lu, "
                    "sizeof("CODEGEN_STRUCT"), offsetof(" CODEGEN_STRUCT ", position));\n"
                    "}\n\n",
                options.prefix,
                options.max_lex_tokens,
                options.max_token_len,
                options.max_lex_state_depth,
                options.parsing_stack_n);
    }
    else
    {
        fprintf(fp, "void* %s_allocate_buffers()\n"
                    "{\n"
                    "    return parser_allocate_buffers(%lu, %lu, %lu, %lu, "
                    "sizeof(" CODEGEN_STRUCT "), sizeof(" CODEGEN_STRUCT "));\n"
                    "}\n\n",
                options.prefix,
                options.max_lex_tokens,
                options.max_token_len,
                options.max_lex_state_depth,
                options.parsing_stack_n);
    }

    fprintf(fp, "void %s_free_buffers(void* self)\n"
                "{\n"
                "    parser_free_buffers((ParserBuffers*)self);\n"
                "}\n\n",
                options.prefix);

    fprintf(fp, "static " CODEGEN_UNION " t;\n"
                "typeof(t.%s) %s_parse(void* buffers, const char* input)\n"
                "{\n"
                "    parser_reset_buffers((ParserBuffers*)buffers);\n"
                "    \n"
                "    uint64_t input_len = strlen(input);\n"
                "    int32_t output_idx = parser_parse_lr(&parser, GEN_parsing_table,\n"
                "         (ParserBuffers*)buffers, input, input_len);\n"
                "    \n"
                "    if (output_idx < 0) return (typeof(t.%s))0;\n"
                "    return ((" CODEGEN_UNION "*)((ParserBuffers*)buffers)->value_table)[output_idx].%s;\n"
                "}\n"
                "typeof(t.%s) %s_parse_len(void* buffers, const char* input, uint64_t input_len)\n"
                "{\n"
                "    parser_reset_buffers((ParserBuffers*)buffers);\n"
                "    \n"
                "    int32_t output_idx = parser_parse_lr(&parser, GEN_parsing_table,\n"
                "         (ParserBuffers*)buffers, input, input_len);\n"
                "    \n"
                "    if (output_idx < 0) return (typeof(t.%s))0;\n"
                "    return ((" CODEGEN_UNION "*)((ParserBuffers*)buffers)->value_table)[output_idx].%s;\n"
                "}\n"
                "\n"
                "#endif\n",
            _start->key, options.prefix,
            _start->key, _start->key,
            _start->key, options.prefix,
            _start->key, _start->key);

    if (_bottom)
    {
        fputs(_bottom->value, fp);
        fputc('\n', fp);
    }

    EXIT_IF_ERRORS;
    DESTROY_ALL;
    return error;
}
