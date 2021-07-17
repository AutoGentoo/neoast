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

#include "codegen/codegen.h"
#include <string.h>
#include <stdlib.h>
#include <parsergen/canonical_collection.h>
#include <stdarg.h>
#include <assert.h>
#include "codegen/builtin_lexer/builtin_lexer.h"
#include "grammar.h"

#define WS_X "[\\s]+"
#define WS_OPT "[\\s]*"
#define ID_X "[A-Za-z_][A-Za-z0-9_]*"
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
        "}",
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

    lex_val->key_val = key_val_build(NULL, KEY_VAL_OPTION,
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
    lex_val->key_val = key_val_build(NULL, KEY_VAL_MACRO, key, value);
    return TOK_OPTION;
}

static int32_t ll_token(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(NULL, KEY_VAL_TOKEN,
                                     NULL,
                                     strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_token_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 7, '\'');
    lex_val->key_val = key_val_build(NULL, KEY_VAL_TOKEN_ASCII,
                                     NULL, get_ascii_token_name(find_token_start[1]));
    return TOK_OPTION;
}

static int32_t ll_start(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(NULL, KEY_VAL_START,
                                     strndup(type_start + 1, type_end - type_start - 1),
                                     strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_state(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(NULL, KEY_VAL_START,
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

    lex_val->key_val = key_val_build(NULL, KEY_VAL_TYPE,
                                     strndup(type_start + 1, type_end - type_start - 1),
                                     strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_destructor(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');

    lex_val->key_val = key_val_build(NULL, KEY_VAL_DESTRUCTOR,
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

    lex_val->key_val = key_val_build(NULL, KEY_VAL_TOKEN_TYPE,
                                     strndup(type_start + 1, type_end - type_start - 1),
                                     strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_left(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 6;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(NULL, KEY_VAL_LEFT, NULL, strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_left_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 6, '\'');
    lex_val->key_val = key_val_build(NULL, KEY_VAL_LEFT, NULL, get_ascii_token_name(find_token_start[1]));
    return TOK_OPTION;
}

static int32_t ll_right(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(NULL, KEY_VAL_RIGHT, NULL, strdup(find_token_start));
    return TOK_OPTION;
}

static int32_t ll_right_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = strchr(lex_text + 7, '\'');
    lex_val->key_val = key_val_build(NULL, KEY_VAL_RIGHT, NULL, get_ascii_token_name(find_token_start[1]));
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

static int32_t ll_g_tok(const char* lex_text, CodegenUnion* lex_val)
{
    lex_val->token = build_token(NULL, strdup(lex_text));
    return TOK_G_TOK;
}

static int32_t ll_g_tok_ascii(const char* lex_text, CodegenUnion* lex_val)
{
    lex_val->token = build_token_ascii(NULL, lex_text[1]);
    return TOK_G_TOK;
}

static int32_t ll_match_brace(const char* lex_text,
                              void* lex_val,
                              uint32_t len,
                              ParsingStack* lexer_state,
                              TokenPosition* position)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;
    (void) position;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_MATCH_BRACE);
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.counter = 1;
    return -1;
}

static int32_t
ll_lex_state(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* ll_state, PositionData* position)
{
    (void) len;
    (void) position;

    const char* start_ptr = strchr(lex_text, '<');
    const char* end_ptr = strchr(lex_text, '>');

    lex_val->string = strndup(start_ptr + 1, end_ptr - start_ptr - 1);
    NEOAST_STACK_PUSH(ll_state, LEX_STATE_LEX_STATE);

    return TOK_LEX_STATE;
}

static int32_t ll_lex_state_exit(const char* lex_text,
                                 CodegenUnion* lex_val,
                                 uint32_t len,
                                 ParsingStack* ll_state,
                                 PositionData* position)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;
    (void) position;

    NEOAST_STACK_POP(ll_state);
    return TOK_END_STATE;
}

static int32_t ll_build_regex(const char* lex_text,
                              void* lex_val,
                              uint32_t len,
                              ParsingStack* lexer_state,
                              PositionData* position)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;
    (void) position;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_REGEX);
    regex_buffer.s = 256;
    regex_buffer.buffer = malloc(regex_buffer.s);
    regex_buffer.n = 0;
    regex_buffer.counter = 1;
    return -1;
}

static int32_t ll_decrement_brace(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* lexer_state,
                                  TokenPosition* position)
{
    (void) lex_text;
    (void) len;
    (void) position;

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

static int32_t
ll_add_to_buffer(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state, TokenPosition* position)
{
    (void) lex_val;
    (void) lexer_state;
    (void) position;

    if (len + brace_buffer.n >= brace_buffer.s)
    {
        brace_buffer.s *= 2;
        brace_buffer.buffer = realloc(brace_buffer.buffer, brace_buffer.s);
    }

    strncpy(brace_buffer.buffer + brace_buffer.n, lex_text, len);
    brace_buffer.n += len;
    return -1;
}

static int32_t ll_regex_quote(const char* lex_text, CodegenUnion* lex_val, uint32_t len, ParsingStack* lexer_state,
                              TokenPosition* position)
{
    (void) lex_text;
    (void) len;
    (void) position;

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

static int32_t ll_regex_add_to_buffer(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state,
                                      TokenPosition* position)
{
    (void) lex_val;
    (void) lexer_state;
    (void) position;

    while (len + regex_buffer.n >= regex_buffer.s)
    {
        regex_buffer.s *= 2;
        regex_buffer.buffer = realloc(regex_buffer.buffer, regex_buffer.s);
    }

    memcpy(regex_buffer.buffer + regex_buffer.n, lex_text, len);
    regex_buffer.n += len;
    return -1;
}

static int32_t ll_regex_enter_comment(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state,
                                      TokenPosition* position)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;
    (void) position;

    NEOAST_STACK_PUSH(lexer_state, LEX_STATE_COMMENT);
    return -1;
}

static int32_t ll_regex_exit_comment(const char* lex_text, void* lex_val, uint32_t len, ParsingStack* lexer_state,
                                     TokenPosition* position)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;
    (void) position;

    NEOAST_STACK_POP(lexer_state);
    return -1;
}

static LexerRule ll_rules_s0[] = {
        {.regex = "[\\n ]+"}, // skip
        {.regex = "//[^\\n]*\\n"},
        {.regex = "/\\*", .expr = ll_regex_enter_comment},
        {.regex = "%%", .expr = (lexer_expr) ll_enter_grammar},
        {.regex = "==", .expr = (lexer_expr) ll_enter_lex},
        {.regex = "%option" WS_X ID_X"=\"[^\"]*\"", .expr = (lexer_expr) ll_option},
        {.regex = "%token" WS_X ID_X, .expr = (lexer_expr) ll_token},
        {.regex = "%token" WS_X ASCII, .expr = (lexer_expr) ll_token_ascii},
        {.regex = "%token" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_token_with_type},
        {.regex = "%start" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_start},
        {.regex = "%state" WS_X ID_X, .expr = (lexer_expr) ll_state},
        {.regex = "%type" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_type},
        {.regex = "%left" WS_X ID_X, .expr = (lexer_expr) ll_left},
        {.regex = "%left" WS_X ASCII, .expr = (lexer_expr) ll_left_ascii},
        {.regex = "%right" WS_X ID_X, .expr = (lexer_expr) ll_right},
        {.regex = "%right" WS_X ASCII, .expr = (lexer_expr) ll_right_ascii},
        {.regex = "%top", .tok = TOK_HEADER},
        {.regex = "%bottom", .tok = TOK_BOTTOM},
        {.regex = "%union", .tok = TOK_UNION},
        {.regex = "%destructor" WS_OPT "<" ID_X ">", .expr = (lexer_expr) ll_destructor},
        {.regex = "\\{", .expr = ll_match_brace},
        {.regex = "\\+" ID_X WS_X "[^\n]+", .expr = (lexer_expr) ll_macro},
};

static LexerRule ll_rules_lex[] = {
        {.regex = "[\\n ]+"}, // skip
        {.regex = "[/][/][^\\n]*\\n"},
        {.regex = "[/]\\*", .expr = ll_regex_enter_comment},
        {.regex = "==", .expr = (lexer_expr) ll_exit_state},
        {.regex = "<" ID_X ">" WS_OPT "\\{", .expr = (lexer_expr) ll_lex_state},
        {.regex = "\"", .expr = (lexer_expr) ll_build_regex},
        {.regex = "[{]", .expr = (lexer_expr) ll_match_brace},
};

static LexerRule ll_rules_grammar[] = {
        {.regex = "[\\n ]+"}, // skip
        {.regex = "[/][/][^\\n]*\\n"},
        {.regex = "[/]\\*", .expr = ll_regex_enter_comment},
        {.regex = "%%", .expr = (lexer_expr) ll_exit_state},
        {.regex = ID_X WS_OPT ":", .expr = (lexer_expr) ll_g_rule},
        {.regex = ASCII, .expr = (lexer_expr) ll_g_tok_ascii},
        {.regex = "[{]", .expr = (lexer_expr) ll_match_brace},
        {.regex = ID_X, .expr = (lexer_expr) ll_g_tok},
        {.regex = "\\|", .tok = TOK_G_OR},
        {.regex = ";", .tok = TOK_G_TERM},
};

static LexerRule ll_rules_match_brace[] = {
        {.regex = "'[\\x00-\\x7F]'", .expr = ll_add_to_buffer},
        {.regex = "(?:\"[^\"]*\")", .expr = ll_add_to_buffer},
        {.regex = "[}]", .expr = (lexer_expr) ll_decrement_brace},
        {.regex = "[{]", .expr = ll_increment_brace},
        {.regex = "(?:[^}{\"\']+)", .expr = ll_add_to_buffer},
};

static LexerRule ll_rules_regex[] = {
        {.regex = "\"", .expr = (lexer_expr) ll_regex_quote},
        {.regex = "[^\"]+", .expr = ll_regex_add_to_buffer}
};

static LexerRule ll_rules_lex_state[] = {
        {.regex = "[\\n ]+"}, // skip
        {.regex = "[/][/][^\\n]*\\n"},
        {.regex = "[/]\\*", .expr = ll_regex_enter_comment},
        {.regex = "[}]", .expr = (lexer_expr) ll_lex_state_exit},
        {.regex = "\"", .expr = (lexer_expr) ll_build_regex},
        {.regex = "[{]", .expr = (lexer_expr) ll_match_brace},
};

static LexerRule ll_rules_comment[] = {
        {.regex = "\\*/", .expr = ll_regex_exit_comment},
        {.regex = "[^\\*]+"},
        {.regex = "\\*"},
};

static const LexerRule* ll_rules[] = {
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
        {TOK_HEADER,            TOK_G_ACTION},
        {TOK_UNION,             TOK_G_ACTION},
        {TOK_BOTTOM,            TOK_G_ACTION},
        {TOK_DESTRUCTOR,        TOK_G_ACTION},

        /* TOK_GG_HEADER */
        {TOK_GG_KEY_VALS},
        {TOK_GG_KEY_VALS,       TOK_GG_HEADER},

        /* Lexer Rules */
        // TOK_GG_LEX_RULE =>
        {TOK_REGEX_RULE,        TOK_G_ACTION},
        {TOK_LEX_STATE,         TOK_GG_LEX_RULES,     TOK_END_STATE},

        // TOK_GG_LEX_RULES =>
        {TOK_GG_LEX_RULE},
        {TOK_GG_LEX_RULE,       TOK_GG_LEX_RULES},

        /* Grammar rules */
        // TOK_GG_TOKENS =>
        {TOK_G_TOK},
        {TOK_G_TOK,             TOK_GG_TOKENS},

        // TOK_GG_SINGLE_GRAMMAR =>
        {TOK_GG_TOKENS,         TOK_G_ACTION},

        // TOK_GG_MULTI_GRAMMAR =>
        {TOK_GG_SINGLE_GRAMMAR},
        {TOK_GG_SINGLE_GRAMMAR, TOK_G_OR,             TOK_GG_MULTI_GRAMMAR},
        {TOK_G_ACTION}, // empty rule

        // TOK_GG_GRAMMAR =>
        {TOK_G_EXPR_DEF,        TOK_GG_MULTI_GRAMMAR, TOK_G_TERM},

        // TOK_GG_GRAMMARS =>
        {TOK_GG_GRAMMAR},
        {TOK_GG_GRAMMAR,        TOK_GG_GRAMMARS},

        // TOK_GG_FILE =>
        {TOK_GG_HEADER,         TOK_DELIMITER,        TOK_GG_LEX_RULES, TOK_DELIMITER, TOK_DELIMITER, TOK_GG_GRAMMARS, TOK_DELIMITER}
};

static void gg_key_val_add_next(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = args[0].key_val;
    dest->key_val->next = args[1].key_val;
}

static void gg_build_header(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(NULL, KEY_VAL_TOP, NULL, args[1].string);
}

static void gg_build_union(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(NULL, KEY_VAL_UNION, NULL, args[1].string);
}

static void gg_build_bottom(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = key_val_build(NULL, KEY_VAL_BOTTOM, NULL, args[1].string);
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
    dest->l_rule->state_rules = NULL;
    dest->l_rule->next = NULL;
}

static void gg_build_l_rule_2(CodegenUnion* dest, CodegenUnion* args)
{
    dest->l_rule = args[0].l_rule;
    assert(dest->l_rule->next == NULL);
    dest->l_rule->next = args[1].l_rule;
}

static void gg_build_l_rule_3(CodegenUnion* dest, CodegenUnion* args)
{
    // Every rule here is part of this state
    dest->l_rule = malloc(sizeof(struct LexerRuleProto));
    dest->l_rule->regex = NULL;
    dest->l_rule->function = NULL;
    dest->l_rule->lexer_state = args[0].string;
    dest->l_rule->state_rules = args[1].l_rule;
    dest->l_rule->next = NULL;
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
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[2], .expr = (parser_expr) gg_build_header},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[3], .expr = (parser_expr) gg_build_union},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[4], .expr = (parser_expr) gg_build_bottom},
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[5], .expr = (parser_expr) gg_build_destructor},
        {.token=TOK_GG_HEADER, .tok_n=1, .grammar=grammars[6]},
        {.token=TOK_GG_HEADER, .tok_n=2, .grammar=grammars[7], .expr = (parser_expr) gg_key_val_add_next},
        {.token=TOK_GG_LEX_RULE, .tok_n=2, .grammar=grammars[8], .expr = (parser_expr) gg_build_l_rule_1},
        {.token=TOK_GG_LEX_RULE, .tok_n=3, .grammar=grammars[9], .expr = (parser_expr) gg_build_l_rule_3},
        {.token=TOK_GG_LEX_RULES, .tok_n=1, .grammar=grammars[10]},
        {.token=TOK_GG_LEX_RULES, .tok_n=2, .grammar=grammars[11], .expr = (parser_expr) gg_build_l_rule_2},
        {.token=TOK_GG_TOKENS, .tok_n=1, .grammar=grammars[12]},
        {.token=TOK_GG_TOKENS, .tok_n=2, .grammar=grammars[13], .expr = (parser_expr) gg_build_tok},
        {.token=TOK_GG_SINGLE_GRAMMAR, .tok_n=2, .grammar=grammars[14], .expr = (parser_expr) gg_build_single},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=1, .grammar=grammars[15]},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=3, .grammar=grammars[16], .expr = (parser_expr) gg_build_multi},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=1, .grammar=grammars[17], .expr = (parser_expr) gg_build_multi_empty},
        {.token=TOK_GG_GRAMMAR, .tok_n=3, .grammar=grammars[18], .expr = (parser_expr) gg_build_grammar},
        {.token=TOK_GG_GRAMMARS, .tok_n=1, .grammar=grammars[19]},
        {.token=TOK_GG_GRAMMARS, .tok_n=2, .grammar=grammars[20], .expr = (parser_expr) gg_build_grammars},
        {.token=TOK_GG_FILE, .tok_n=7, .grammar=grammars[21], .expr = (parser_expr) gg_build_file},
};

uint8_t precedence_table[TOK_AUGMENT] = {0};

static void ll_error(const char* input, const TokenPosition* position)
{
    (void) input;

    emit_error(position, "Failed to match token");
}

int gen_parser_init(GrammarParser* self, void** lexer_ptr)
{
    self->grammar_rules = gg_rules;
    self->grammar_n = NEOAST_ARR_LEN(gg_rules);
    self->action_token_n = TOK_GG_FILE;
    self->token_n = TOK_AUGMENT;
    self->token_names = tok_names_errors;
    self->destructors = NULL;
    self->ascii_mappings = NULL;
    self->lexer_opts = 0;

    *lexer_ptr = builtin_lexer_new(ll_rules,
                                   ll_rules_n,
                                   NEOAST_ARR_LEN(ll_rules_n),
                                   NULL);

    precedence_table[TOK_G_OR] = PRECEDENCE_LEFT;

    // Generate the parsing table
    CanonicalCollection* cc = canonical_collection_init(self);
    canonical_collection_resolve(cc, LALR_1);

    uint8_t error;
    GEN_parsing_table = canonical_collection_generate(cc, precedence_table, &error);
    canonical_collection_free(cc);

    if (error)
    {
        fprintf(stderr, "Failed to generate parsing table for codegen grammar!\n");
        free(GEN_parsing_table);
        return 1;
    }

    return 0;
}

void emit_warning(const TokenPosition* position, const char* message, ...)
{
    (void) position;

    fprintf(stderr, "Warning: ");

    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fputc('\n', stderr);
}

static int error_counter = 0;

void emit_error(const TokenPosition* position, const char* message, ...)
{
    (void) position;

    fprintf(stderr, "Error: ");

    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fputc('\n', stderr);
    error_counter++;
}

int has_errors()
{
    return error_counter;
}
