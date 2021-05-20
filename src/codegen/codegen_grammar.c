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

#define _GNU_SOURCE

#include "lexer.h"
#include "codegen.h"
#include <string.h>
#include <stdlib.h>
#include <parsergen/canonical_collection.h>

#define WS_X "[\\s]+"
#define WS_OPT "[\\s]*"
#define ID_X "[A-z][A-z0-9_]*"
#define ASCII "'[\\x20-\\x7E]'"

uint32_t* GEN_parsing_table = NULL;
const char* tok_names_errors[] = {
        "eof",
        "option",
        "header",
        "bottom",
        "union",
        "destructor",
        "==",
        "lex_state",
        "regex",
        "expr_def",
        "grammar_token",
        "|",
        ";",
        "{grammar action}",
        "file",
        "key_value",
        "header",
        "lexer_rule",
        "lexer_rules",
        "grammar_rules",
        "grammar_rule",
        "grammar_tokens",
        "grammar",
        "grammars",
        "augment rule"
};

struct LexerTextBuffer
{
    int32_t counter;
    uint32_t s;
    uint32_t n;
    char* buffer;
};

static __thread struct LexerTextBuffer brace_buffer = {0};
static __thread struct LexerTextBuffer regex_buffer = {0};

static inline
struct KeyVal* key_val_build(key_val_t type, char* key, char* value)
{
    struct KeyVal* self = malloc(sizeof(struct KeyVal));
    self->next = NULL;
    self->type = type;
    self->key = key;
    self->value = value;
    self->options = 0;

    return self;
}
static struct Token* token_build(char* name)
{
    struct Token* self = malloc(sizeof(struct Token));
    self->name = name;
    self->next = NULL;
    return self;
}

static int32_t ll_enter_lex(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_PUSH(ll_state, LEX_STATE_LEXER_RULES);
    return TOK_DELIMITER;
}
static int32_t ll_enter_grammar(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_PUSH(ll_state, LEX_STATE_GRAMMAR_RULES);
    return TOK_DELIMITER;
}
static int32_t ll_exit_state(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_POP(ll_state);
    return TOK_DELIMITER;
}
static int32_t ll_option(const char* lex_text, CodegenUnion* lex_val)
{
    // Skip over '%option '
    const char* text_skip = lex_text + 8;
    char* divider_1 = strchr(text_skip, '=');
    char* divider_2 = strchr(divider_1 + 2, '"');

    lex_val->key_val = key_val_build(KEY_VAL_OPTION,
                                     strndup(text_skip, divider_1 - text_skip),
                                     strndup(divider_1 + 2, divider_2 - divider_1 - 2));

    return TOK_OPTION;
}

static int32_t ll_macro(const char* lex_text, CodegenUnion* lex_val, uint32_t len)
{
    // Find the white space delimiter
    const char* split = strchr(lex_text + 1, ' ');
    if (!split)
    {
        split = lex_text + len;
    }
    char* key = strndup(lex_text + 1, split - lex_text - 1);

    // Find the start of the regex rule
    while (*split == ' ')
    {
        split++;
    }

    char* value = strdup(split);
    lex_val->key_val = key_val_build(KEY_VAL_MACRO, key, value);
    return TOK_OPTION;
}
static int32_t ll_token(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN,
                                     strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static int32_t ll_token_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 7, '\'');
    char* key = NULL;
    asprintf(&key, "ASCII_CHAR_0x%02X", find_token_start[1]);

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN_ASCII,
                                     key, NULL);
    lex_val->key_val->options = (uint8_t)find_token_start[1];
    return TOK_OPTION;
}
static int32_t ll_start(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_START,
                                     strdup(find_token_start),
                                     strndup(type_start + 1, type_end - type_start - 1));
    return TOK_OPTION;
}
static int32_t ll_state(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_START,
                                     strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static int32_t ll_type(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TYPE,
                                     strdup(find_token_start),
                                     strndup(type_start + 1, type_end - type_start - 1));
    return TOK_OPTION;
}
static int32_t ll_destructor(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');

    lex_val->key_val = key_val_build(KEY_VAL_DESTRUCTOR,
                                     strndup(type_start + 1, type_end - type_start - 1),
                                     NULL);
    return TOK_DESTRUCTOR;
}
static int32_t ll_token_with_type(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 6, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN_TYPE,
                                     strdup(find_token_start),
                                     strndup(type_start + 1, type_end - type_start - 1));
    return TOK_OPTION;
}
static int32_t ll_token_with_type_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 6, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = strchr(type_end + 1, '\'');
    char* key = NULL;
    asprintf(&key, "ASCII_CHAR_0x%02X", find_token_start[1]);

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN_TYPE_ASCII,
                                     key,
                                     strndup(type_start + 1, type_end - type_start - 1));
    lex_val->key_val->options = (uint8_t)find_token_start[1];
    return TOK_OPTION;
}
static int32_t ll_left(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 6;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_LEFT, strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static int32_t ll_left_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 6, '\'');
    char* key = NULL;
    asprintf(&key, "ASCII_CHAR_0x%02X", find_token_start[1]);

    lex_val->key_val = key_val_build(KEY_VAL_LEFT, key, NULL);
    lex_val->key_val->options = (uint8_t)find_token_start[1];
    return TOK_OPTION;
}
static int32_t ll_right(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_RIGHT, strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static int32_t ll_right_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 7, '\'');
    char* key = NULL;
    asprintf(&key, "ASCII_CHAR_0x%02X", find_token_start[1]);

    lex_val->key_val = key_val_build(KEY_VAL_RIGHT, key, NULL);
    lex_val->key_val->options = (uint8_t)find_token_start[1];
    return TOK_OPTION;
}

static int32_t ll_g_rule(const char* lex_text, CodegenUnion* lex_val)
{
    const char* gg_rule_end = lex_text + 1;
    while (*gg_rule_end && *gg_rule_end != ' ' && *gg_rule_end != ':')
    {
        gg_rule_end++;
    }
    lex_val->string = strndup(lex_text, gg_rule_end - lex_text);
    return TOK_G_EXPR_DEF;
}
static int32_t ll_g_tok (const char* lex_text, CodegenUnion* lex_val)
{
    lex_val->token = token_build(strdup(lex_text));
    return TOK_G_TOK;
}
static int32_t ll_g_tok_ascii (const char* lex_text, CodegenUnion* lex_val)
{
    char* token = NULL;
    asprintf(&token, "ASCII_CHAR_0x%02X", lex_text[1]);
    lex_val->token = token_build(token);
    return TOK_G_TOK;
}

static int32_t ll_match_brace(const char* lex_text,
                              void* lex_val,
                              uint32_t len,
                              ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_MATCH_BRACE);
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.counter = 1;
    return -1;
}

static int32_t ll_lex_state(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* ll_state)
{
    (void) len;

    const char* start_ptr = strchr(lex_text, '<');
    const char* end_ptr = strchr(lex_text, '>');

    lex_val->string = strndup(start_ptr, end_ptr - start_ptr);
    NEOAST_STACK_PUSH(ll_state, LEX_STATE_LEX_STATE);

    return TOK_LEX_STATE;
}

static int32_t ll_lex_state_exit(const char* lex_text,
                                 CodegenUnion* lex_val,
                                 uint32_t len,
                                 ParsingStack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_POP(ll_state);
    return -1;
}

static int32_t ll_build_regex(const char* lex_text,
                              void* lex_val,
                              uint32_t len,
                              ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_REGEX);
    regex_buffer.s = 256;
    regex_buffer.buffer = malloc(regex_buffer.s);
    regex_buffer.n = 0;
    regex_buffer.counter = 1;
    return -1;
}

static int32_t ll_decrement_brace(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) len;

    brace_buffer.counter--;
    if (brace_buffer.counter <= 0)
    {
        brace_buffer.buffer[brace_buffer.n++] = 0;
        NEOAST_STACK_POP(lexer_state);
        lex_val->string = strndup(brace_buffer.buffer, brace_buffer.n);
        free(brace_buffer.buffer);
        brace_buffer.buffer = NULL;
        return TOK_G_ACTION;
    }
    else
    {
        brace_buffer.buffer[brace_buffer.n++] = '}';
    }
    return -1;
}
static int32_t ll_increment_brace()
{
    brace_buffer.counter++;
    brace_buffer.buffer[brace_buffer.n++] = '{';
    return -1;
}
static int32_t ll_add_to_buffer(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_val;
    (void) lexer_state;

    if (len + brace_buffer.n >= brace_buffer.s)
    {
        brace_buffer.s *= 2;
        brace_buffer.buffer = realloc(brace_buffer.buffer, brace_buffer.s);
    }

    strncpy(brace_buffer.buffer + brace_buffer.n, lex_text, len);
    brace_buffer.n += len;
    return -1;
}

static int32_t ll_regex_quote(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) len;

    // Make sure this quote is not escaped
    if (!regex_buffer.n || regex_buffer.buffer[regex_buffer.n - 1] != '\\')
    {
        // This is the string terminator
        lex_val->string = strndup(regex_buffer.buffer, regex_buffer.n);
        free(regex_buffer.buffer);
        regex_buffer.buffer = NULL;
        regex_buffer.s = 0;
        regex_buffer.n = 0;
        NEOAST_STACK_POP(lexer_state);
        return TOK_REGEX_RULE;
    }

    // quote was escaped
    regex_buffer.buffer[regex_buffer.n++] = '\"';
    return -1;
}

static int32_t ll_regex_add_to_buffer(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_val;
    (void) lexer_state;

    while (len + regex_buffer.n >= regex_buffer.s)
    {
        regex_buffer.s *= 2;
        regex_buffer.buffer = realloc(regex_buffer.buffer, regex_buffer.s);
    }

    memcpy(regex_buffer.buffer + regex_buffer.n, lex_text, len);
    regex_buffer.n += len;
    return -1;
}

static int32_t ll_regex_enter_comment(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_COMMENT);
    return -1;
}

static int32_t ll_regex_exit_comment(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    NEOAST_STACK_POP(lexer_state);
    return -1;
}

static LexerRule ll_rules_s0[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "//[^\n]*\n"},
        {.regex_raw = "/\\*", .expr = ll_regex_enter_comment},
        {.regex_raw = "%%", .expr = (lexer_expr) ll_enter_grammar},
        {.regex_raw = "==", .expr = (lexer_expr) ll_enter_lex},
        {.regex_raw = "%option" WS_X ID_X"=\"[^\"]*\"", .expr = (lexer_expr) ll_option},
        {.regex_raw = "%token" WS_X ID_X, .expr = (lexer_expr) ll_token},
        {.regex_raw = "%token" WS_X ASCII, .expr = (lexer_expr) ll_token_ascii},
        {.regex_raw = "%token" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_token_with_type},
        {.regex_raw = "%token" WS_OPT "<"ID_X">" WS_X ASCII, .expr = (lexer_expr) ll_token_with_type_ascii},
        {.regex_raw = "%start" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_start},
        {.regex_raw = "%state" WS_X ID_X, .expr = (lexer_expr) ll_state},
        {.regex_raw = "%type" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_type},
        {.regex_raw = "%left" WS_X ID_X, .expr = (lexer_expr) ll_left},
        {.regex_raw = "%left" WS_X ASCII, .expr = (lexer_expr) ll_left_ascii},
        {.regex_raw = "%right" WS_X ID_X, .expr = (lexer_expr) ll_right},
        {.regex_raw = "%right" WS_X ASCII, .expr = (lexer_expr) ll_right_ascii},
        {.regex_raw = "%top", .tok = TOK_HEADER},
        {.regex_raw = "%bottom", .tok = TOK_BOTTOM},
        {.regex_raw = "%union", .tok = TOK_UNION},
        {.regex_raw = "%destructor" WS_OPT "<" ID_X ">", .expr = (lexer_expr) ll_destructor},
        {.regex_raw = "{", .expr = ll_match_brace},
        {.regex_raw = "\\+" ID_X WS_X "[^\n]+", .expr = (lexer_expr) ll_macro},
};

static LexerRule ll_rules_lex[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "//[^\n]*\n"},
        {.regex_raw = "/\\*", .expr = ll_regex_enter_comment},
        {.regex_raw = "==", .expr = (lexer_expr) ll_exit_state},
        {.regex_raw = "<" ID_X ">" WS_OPT "{", .expr = (lexer_expr) ll_lex_state},
        {.regex_raw = "\"", .expr = (lexer_expr) ll_build_regex},
        {.regex_raw = "{", .expr = (lexer_expr) ll_match_brace},
};

static LexerRule ll_rules_grammar[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "//[^\n]*\n"},
        {.regex_raw = "/\\*", .expr = ll_regex_enter_comment},
        {.regex_raw = "%%", .expr = (lexer_expr) ll_exit_state},
        {.regex_raw = ID_X WS_OPT ":", .expr = (lexer_expr) ll_g_rule},
        {.regex_raw = ASCII, .expr = (lexer_expr) ll_g_tok_ascii},
        {.regex_raw = "{", .expr = (lexer_expr) ll_match_brace},
        {.regex_raw = ID_X, .expr = (lexer_expr) ll_g_tok},
        {.regex_raw = "\\|", .tok = TOK_G_OR},
        {.regex_raw = ";", .tok = TOK_G_TERM},
};

static LexerRule ll_rules_match_brace[] = {
        {.regex_raw = "'[\\x00-\\x7F]'", .expr = ll_add_to_buffer},
        {.regex_raw = "(\"[^\"]*\")", .expr = ll_add_to_buffer},
        {.regex_raw = "}", .expr = (lexer_expr) ll_decrement_brace},
        {.regex_raw = "{", .expr = ll_increment_brace},
        {.regex_raw = "([^}{\"\']+)", .expr = ll_add_to_buffer},
};

static LexerRule  ll_rules_regex[] = {
        {.regex_raw = "\"", .expr = (lexer_expr) ll_regex_quote},
        {.regex_raw = "[^\"]+", .expr = ll_regex_add_to_buffer}
};

static LexerRule ll_rules_lex_state[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "//[^\n]*\n"},
        {.regex_raw = "/\\*", .expr = ll_regex_enter_comment},
        {.regex_raw = "==", .expr = (lexer_expr) ll_exit_state},
        {.regex_raw = "}", .expr = (lexer_expr) ll_lex_state_exit},
        {.regex_raw = "\"", .expr = (lexer_expr) ll_build_regex},
        {.regex_raw = "{", .expr = (lexer_expr) ll_match_brace},
};

static LexerRule ll_rules_comment[] = {
        {.regex_raw = "\\*/", .expr = ll_regex_exit_comment},
        {.regex_raw = "[^\\*]+"},
        {.regex_raw = "\\*"},
};

static LexerRule* ll_rules[] = {
        ll_rules_s0,
        ll_rules_lex,
        ll_rules_grammar,
        ll_rules_match_brace,
        ll_rules_regex,
        ll_rules_lex_state,
        ll_rules_comment,
};

static uint32_t ll_rules_n[] = {
        NEOAST_ARR_LEN(ll_rules_s0),
        NEOAST_ARR_LEN(ll_rules_lex),
        NEOAST_ARR_LEN(ll_rules_grammar),
        NEOAST_ARR_LEN(ll_rules_match_brace),
        NEOAST_ARR_LEN(ll_rules_regex),
        NEOAST_ARR_LEN(ll_rules_lex_state),
        NEOAST_ARR_LEN(ll_rules_comment),
};

const uint32_t grammars[][7] = {
        /* Augmented rule */
        {TOK_GG_FILE},

        /* TOK_GG_KEY_VALS => */
        {TOK_OPTION},
        {TOK_HEADER, TOK_G_ACTION},
        {TOK_UNION, TOK_G_ACTION},
        {TOK_BOTTOM, TOK_G_ACTION},
        {TOK_DESTRUCTOR, TOK_G_ACTION},

        /* TOK_GG_HEADER */
        {TOK_GG_KEY_VALS},
        {TOK_GG_KEY_VALS, TOK_GG_HEADER},

        /* Lexer Rules */
        {TOK_REGEX_RULE, TOK_G_ACTION},
        {TOK_GG_LEX_RULE},
        {TOK_GG_LEX_RULE, TOK_GG_LEX_RULES},
        {TOK_LEX_STATE, TOK_GG_LEX_RULES},

        /* Grammar rules */
        // TOK_GG_TOKENS =>
        {TOK_G_TOK},
        {TOK_G_TOK, TOK_GG_TOKENS},

        // TOK_GG_SINGLE_GRAMMAR =>
        {TOK_GG_TOKENS, TOK_G_ACTION},

        // TOK_GG_MULTI_GRAMMAR =>
        {TOK_GG_SINGLE_GRAMMAR},
        {TOK_GG_SINGLE_GRAMMAR, TOK_G_OR, TOK_GG_MULTI_GRAMMAR},
        {TOK_G_ACTION}, // empty rule

        // TOK_GG_GRAMMAR =>
        {TOK_G_EXPR_DEF, TOK_GG_MULTI_GRAMMAR, TOK_G_TERM},

        // TOK_GG_GRAMMARS =>
        {TOK_GG_GRAMMAR},
        {TOK_GG_GRAMMAR, TOK_GG_GRAMMARS},

        // TOK_GG_FILE =>
        {TOK_GG_HEADER, TOK_DELIMITER, TOK_GG_LEX_RULES, TOK_DELIMITER, TOK_DELIMITER, TOK_GG_GRAMMARS, TOK_DELIMITER}
};

static void gg_key_val_add_next(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = args[0].key_val;
    dest->key_val->next = args[1].key_val;
}
static void gg_build_header(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(KEY_VAL_HEADER, NULL, args[1].string);
}
static void gg_build_union(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(KEY_VAL_UNION, NULL, args[1].string);
}
static void gg_build_bottom(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(KEY_VAL_BOTTOM, NULL, args[1].string);
}
static void gg_build_destructor(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = args[0].key_val;
    dest->key_val->value = args[1].string;
}
static void gg_build_l_rule_1(CodegenUnion* dest, CodegenUnion* args)
{
    dest->l_rule = malloc(sizeof(struct LexerRuleProto));
    dest->l_rule->regex = args[0].string;
    dest->l_rule->function = args[1].string;
    dest->l_rule->lexer_state = NULL;
    dest->l_rule->next = NULL;
}
static void gg_build_l_rule_2(CodegenUnion* dest, CodegenUnion* args)
{
    dest->l_rule = args[0].l_rule;
    dest->l_rule->next = args[1].l_rule;
}
static void gg_build_l_rule_3(CodegenUnion* dest, CodegenUnion* args)
{
    // Every rule here is part of this state
    dest->l_rule = args[1].l_rule;
    for (struct LexerRuleProto* iter = args[1].l_rule; iter; iter = iter->next)
    {
        iter->lexer_state = args[0].string;
    }
}
static void gg_build_tok(CodegenUnion* dest, CodegenUnion* args)
{
    dest->token = args[0].token;
    dest->token->next = args[1].token;
}
static void gg_build_single(CodegenUnion* dest, CodegenUnion* args)
{
    dest->g_single_rule = malloc(sizeof(struct GrammarRuleSingleProto));
    dest->g_single_rule->next = NULL;
    dest->g_single_rule->tokens = args[0].token;
    dest->g_single_rule->function = args[1].string;
}
static void gg_build_multi(CodegenUnion* dest, CodegenUnion* args)
{
    dest->g_single_rule = args[0].g_single_rule;
    dest->g_single_rule->next = args[2].g_single_rule;
}
static void gg_build_multi_empty(CodegenUnion* dest, CodegenUnion* args)
{
    dest->g_single_rule = malloc(sizeof(struct GrammarRuleSingleProto));
    dest->g_single_rule->next = NULL;
    dest->g_single_rule->tokens = NULL;
    dest->g_single_rule->function = args[0].string;
}
static void gg_build_grammar(CodegenUnion* dest, CodegenUnion* args)
{
    dest->g_rule = malloc(sizeof(struct GrammarRuleProto));
    dest->g_rule->name = args[0].string;
    dest->g_rule->rules = args[1].g_single_rule;
    dest->g_rule->next = NULL;
}
static void gg_build_grammars(CodegenUnion* dest, CodegenUnion* args)
{
    dest->g_rule = args[0].g_rule;
    dest->g_rule->next = args[1].g_rule;
}
static void gg_build_file(CodegenUnion* dest, CodegenUnion* args)
{
    dest->file = malloc(sizeof(struct File));
    dest->file->header = args[0].key_val;
    dest->file->lexer_rules = args[2].l_rule;
    dest->file->grammar_rules = args[5].g_rule;
}

static const GrammarRule gg_rules[] = {
        {.token=TOK_AUGMENT, .tok_n=1, .grammar=grammars[0]},
        {.token=TOK_GG_KEY_VALS, .tok_n=1, .grammar=grammars[1]},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[2],        .expr = (parser_expr) gg_build_header},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[3],        .expr = (parser_expr) gg_build_union},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[4],        .expr = (parser_expr) gg_build_bottom},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[5],        .expr = (parser_expr) gg_build_destructor},
        {.token=TOK_GG_HEADER, .tok_n=1, .grammar=grammars[6]},
        {.token=TOK_GG_HEADER, .tok_n=2, .grammar=grammars[7],          .expr = (parser_expr) gg_key_val_add_next},
        {.token=TOK_GG_LEX_RULE, .tok_n=2, .grammar=grammars[8],        .expr = (parser_expr) gg_build_l_rule_1},
        {.token=TOK_GG_LEX_RULES, .tok_n=1, .grammar=grammars[9]},
        {.token=TOK_GG_LEX_RULES, .tok_n=2, .grammar=grammars[10],       .expr = (parser_expr) gg_build_l_rule_2},
        {.token=TOK_GG_LEX_RULES, .tok_n=2, .grammar=grammars[11],       .expr = (parser_expr) gg_build_l_rule_3},
        {.token=TOK_GG_TOKENS, .tok_n=1, .grammar=grammars[12]},
        {.token=TOK_GG_TOKENS, .tok_n=2, .grammar=grammars[13],         .expr = (parser_expr) gg_build_tok},
        {.token=TOK_GG_SINGLE_GRAMMAR, .tok_n=2, .grammar=grammars[14], .expr = (parser_expr) gg_build_single},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=1, .grammar=grammars[15]},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=3, .grammar=grammars[16],  .expr = (parser_expr) gg_build_multi},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=1, .grammar=grammars[17],  .expr = (parser_expr) gg_build_multi_empty},
        {.token=TOK_GG_GRAMMAR, .tok_n=3, .grammar=grammars[18],        .expr = (parser_expr) gg_build_grammar},
        {.token=TOK_GG_GRAMMARS, .tok_n=1, .grammar=grammars[19]},
        {.token=TOK_GG_GRAMMARS, .tok_n=2, .grammar=grammars[20],       .expr = (parser_expr) gg_build_grammars},
        {.token=TOK_GG_FILE, .tok_n=7, .grammar=grammars[21],           .expr = (parser_expr) gg_build_file},
};

uint8_t precedence_table[TOK_AUGMENT] = {0};

int gen_parser_init(GrammarParser* self)
{
    self->lexer_rules = ll_rules;
    self->grammar_rules = gg_rules;
    self->lex_state_n = NEOAST_ARR_LEN(ll_rules_n);
    self->lex_n = ll_rules_n;
    self->grammar_n = NEOAST_ARR_LEN(gg_rules);
    self->action_token_n = TOK_GG_FILE;
    self->token_n = TOK_AUGMENT;
    self->token_names = tok_names_errors;
    self->destructors = NULL;
    self->ascii_mappings = NULL;
    self->lexer_opts = 0;

    precedence_table[TOK_G_OR] = PRECEDENCE_LEFT;

    uint32_t regex_init_errors;
    if ((regex_init_errors = parser_init(self)))
    {
        fprintf(stderr,"Failed to initialize %d regular expressions\n", regex_init_errors);
        return 1;
    }

    // Generate the parsing table
    CanonicalCollection* cc = canonical_collection_init(self);
    canonical_collection_resolve(cc, LALR_1);

    GEN_parsing_table = canonical_collection_generate(cc, precedence_table);
    canonical_collection_free(cc);

    return 0;
}

static void
key_val_free(struct KeyVal* self)
{
    struct KeyVal* next;
    while (self)
    {
        next = self->next;
        free(self->key);
        free(self->value);
        free(self);
        self = next;
    }
}

static void
lexer_rule_free(struct LexerRuleProto* self)
{
    struct LexerRuleProto* next;
    while (self)
    {
        next = self->next;
        free(self->function);
        free(self->regex);
        free(self->lexer_state);
        free(self);
        self = next;
    }
}

static void
tokens_free(struct Token* self)
{
    struct Token* next;
    while (self)
    {
        next = self->next;
        free(self->name);
        free(self);
        self = next;
    }
}

static void
grammar_rule_single_free(struct GrammarRuleSingleProto* self)
{
    struct GrammarRuleSingleProto* next;
    while (self)
    {
        next = self->next;
        free(self->function);
        tokens_free(self->tokens);
        free(self);
        self = next;
    }
}

static void
grammar_rule_multi_free(struct GrammarRuleProto* self)
{
    struct GrammarRuleProto* next;
    while (self)
    {
        next = self->next;
        free(self->name);
        grammar_rule_single_free(self->rules);
        free(self);
        self = next;
    }
}

void file_free(struct File* self)
{
    key_val_free(self->header);
    lexer_rule_free(self->lexer_rules);
    grammar_rule_multi_free(self->grammar_rules);
    free(self);
}
