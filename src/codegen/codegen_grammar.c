//
// Created by tumbar on 12/25/20.
//

#include "lexer.h"
#include "codegen.h"
#include <parser.h>
#include <string.h>
#include <stdlib.h>
#include <parsergen/canonical_collection.h>
#include <parsergen/clr_lalr.h>
#include <util/util.h>

#define WS_X "[\\s]+"
#define WS_OPT "[\\s]*"
#define ID_X "[A-z][A-z|0-9]*"

U32* GEN_parsing_table = NULL;
const char* tok_names = "$ohdxei|;{FKHlLGMTSMA";
const char* tok_names_errors[] = {
        "eof",
        "option",
        "header",
        "delimiter",
        "regex",
        "expr_def",
        "grammar_token",
        "grammar_or",
        "grammar_term",
        "grammar_action",
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
    I32 counter;
    U32 s;
    U32 n;
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

    return self;
}
static struct Token* token_build(char* name)
{
    struct Token* self = malloc(sizeof(struct Token));
    self->name = name;
    self->next = NULL;
    return self;
}

static I32 ll_enter_lex(const char* lex_text, void* lex_val, U32 len, Stack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_PUSH(ll_state, LEX_STATE_LEXER_RULES);
    return TOK_DELIMITER;
}
static I32 ll_enter_grammar(const char* lex_text, void* lex_val, U32 len, Stack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_PUSH(ll_state, LEX_STATE_GRAMMAR_RULES);
    return TOK_DELIMITER;
}
static I32 ll_exit_state(const char* lex_text, void* lex_val, U32 len, Stack* ll_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_POP(ll_state);
    return TOK_DELIMITER;
}
static I32 ll_option(const char* lex_text, CodegenUnion* lex_val)
{
    // Skip over '%option '
    const char* text_skip = lex_text + 8;
    char* divider = strchr(text_skip, '=');

    lex_val->key_val = key_val_build(KEY_VAL_OPTION,
                                     strndup(text_skip, text_skip - divider),
                                     strdup(divider + 1));

    return TOK_OPTION;
}
static I32 ll_macro(const char* lex_text, CodegenUnion* lex_val)
{
    // Find the white space delimiter
    char* split = strchr(lex_text, ' ');
    char* key = strndup(lex_text, split - lex_text);

    // Find the start of the regex rule
    while (*split == ' ')
    {
        split++;
    }

    char* value = strdup(split);
    lex_val->key_val = key_val_build(KEY_VAL_MACRO, key, value);
    return TOK_OPTION;
}
static I32 ll_header(const char* lex_text, CodegenUnion* lex_val, U32 len, Stack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_PUSH(lexer_state, LEX_STATE_HEADER);
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.buffer[brace_buffer.n++] = '{';
    brace_buffer.counter = 1;
    return -1;
}
static I32 ll_token(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN,
                                     strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static I32 ll_start(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_START,
                                     strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static I32 ll_type(const char* lex_text, CodegenUnion* lex_val)
{
    const char* type_start = strchr(lex_text + 5, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TYPE,
                                     strndup(type_start + 1, type_end - type_start - 2),
                                     strdup(find_token_start));
    return TOK_OPTION;
}
static I32 ll_token_with_type(const char* lex_text, CodegenUnion* lex_val, U32 len)
{
    const char* type_start = strchr(lex_text + 6, '<');
    const char* type_end = strchr(type_start, '>');
    const char* find_token_start = type_end + 1;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_TOKEN_TYPE,
                                     strndup(type_start + 1, type_end - type_start - 1),
                                     strndup(find_token_start, len - (lex_text - find_token_start)));
    return TOK_OPTION;
}
static I32 ll_left(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 6;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_LEFT, strdup(find_token_start), NULL);
    return TOK_OPTION;
}
static I32 ll_right(const char* lex_text, CodegenUnion* lex_val)
{
    const char* find_token_start = lex_text + 7;
    while (*find_token_start == ' ')
        find_token_start++;

    lex_val->key_val = key_val_build(KEY_VAL_RIGHT, strdup(find_token_start), NULL);
    return TOK_OPTION;
}

static I32 ll_g_rule(const char* lex_text, CodegenUnion* lex_val, U32 len)
{
    lex_val->string = strndup(lex_text, len - 1);
    return TOK_G_EXPR_DEF;
}
static I32 ll_g_tok (const char* lex_text, CodegenUnion* lex_val, U32 len)
{
    lex_val->token = token_build(strndup(lex_text, len));
    return TOK_G_TOK;
}

static I32 ll_match_brace(const char* lex_text,
                        void* lex_val,
                        U32 len,
                        Stack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_PUSH(lexer_state, LEX_STATE_MATCH_BRACE);
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.buffer[brace_buffer.n++] = '{';
    brace_buffer.counter = 1;
    return -1;
}

static I32 ll_build_regex(const char* lex_text,
                          void* lex_val,
                          U32 len,
                          Stack* lexer_state)
{
    (void) lex_text;
    (void) lex_val;
    (void) len;

    STACK_PUSH(lexer_state, LEX_STATE_REGEX);
    regex_buffer.s = 256;
    regex_buffer.buffer = malloc(regex_buffer.s);
    regex_buffer.n = 0;
    regex_buffer.counter = 1;
    return -1;
}

static LexerRule ll_rules_s0[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "%%", .expr = (lexer_expr) ll_enter_grammar},
        {.regex_raw = "==", .expr = (lexer_expr) ll_enter_lex},
        {.regex_raw = "%option" WS_X ID_X"=\"[A-z|0-9_\\-\\+]*\"", .expr = (lexer_expr) ll_option},
        {.regex_raw = "%token" WS_X ID_X, .expr = (lexer_expr) ll_token},
        {.regex_raw = "%start" WS_X ID_X, .expr = (lexer_expr) ll_start},
        {.regex_raw = "%type" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_type},
        {.regex_raw = "%token" WS_OPT "<"ID_X">" WS_X ID_X, .expr = (lexer_expr) ll_token_with_type},
        {.regex_raw = "%left" WS_X ID_X, .expr = (lexer_expr) ll_left},
        {.regex_raw = "%right" WS_X ID_X, .expr = (lexer_expr) ll_right},
        {.regex_raw = "%top" WS_OPT "{", .expr = (lexer_expr) ll_header},
        {.regex_raw = "\\+[A-z_][A-z_0-9]*[\\s]+[^\n]+", .expr = (lexer_expr) ll_macro},
};

static LexerRule ll_rules_lex[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "==", .expr = (lexer_expr) ll_exit_state},
        {.regex_raw = "\"", .expr = (lexer_expr) ll_build_regex},
        {.regex_raw = "{", .expr = (lexer_expr) ll_match_brace},
};

static LexerRule ll_rules_grammar[] = {
        {.regex_raw = "[\n ]+"}, // skip
        {.regex_raw = "%%", .expr = (lexer_expr) ll_exit_state},
        {.regex_raw = "[A-z_]+:", .expr = (lexer_expr) ll_g_rule},
        {.regex_raw = "{", .expr = (lexer_expr) ll_match_brace},
        {.regex_raw = "[A-z][A-z_0-9]*", .expr = (lexer_expr) ll_g_tok},
        {.regex_raw = "\\|", .tok = TOK_G_OR},
        {.regex_raw = ";", .tok = TOK_G_TERM},
};

static I32 ll_decrement_brace(const char* lex_text, CodegenUnion* lex_val, U32 len, Stack* lexer_state)
{
    (void) lex_text;
    (void) len;

    brace_buffer.counter--;
    brace_buffer.buffer[brace_buffer.n++] = '}';
    brace_buffer.buffer[brace_buffer.n++] = 0;
    if (brace_buffer.counter <= 0)
    {
        STACK_POP(lexer_state);
        lex_val->string = strndup(brace_buffer.buffer, brace_buffer.n);
        free(brace_buffer.buffer);
        brace_buffer.buffer = NULL;
        return TOK_G_ACTION;
    }
    return -1;
}
static I32 ll_decrement_brace_header(const char* lex_text, CodegenUnion* lex_val, U32 len, Stack* lexer_state)
{
    (void) lex_text;
    (void) len;

    brace_buffer.counter--;
    brace_buffer.buffer[brace_buffer.n++] = '}';
    if (brace_buffer.counter <= 0)
    {
        STACK_POP(lexer_state);
        lex_val->key_val = key_val_build(KEY_VAL_HEADER, NULL, strndup(brace_buffer.buffer, brace_buffer.n));
        free(brace_buffer.buffer);
        brace_buffer.buffer = NULL;
        return TOK_HEADER;
    }
    return -1;
}
static I32 ll_increment_brace()
{
    brace_buffer.counter++;
    brace_buffer.buffer[brace_buffer.n++] = '{';
    return -1;
}
static I32 ll_add_to_buffer(const char* lex_text, void* lex_val, U32 len, Stack* lexer_state)
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

static LexerRule ll_rules_match_brace[] = {
        {.regex_raw = "}", .expr = (lexer_expr) ll_decrement_brace},
        {.regex_raw = "{", .expr = ll_increment_brace},
        {.regex_raw = "([^}{]+)", .expr = ll_add_to_buffer},
};

static LexerRule ll_rules_header[] = {
        {.regex_raw = "}", .expr = (lexer_expr) ll_decrement_brace_header},
        {.regex_raw = "{", .expr = ll_increment_brace},
        {.regex_raw = "([^}{]+)", .expr = ll_add_to_buffer},
};


static I32 ll_regex_quote(const char* lex_text, CodegenUnion* lex_val, U32 len, Stack* lexer_state)
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
        STACK_POP(lexer_state);
        return TOK_REGEX_RULE;
    }

    // quote was escaped
    regex_buffer.buffer[regex_buffer.n++] = '\"';
    return -1;
}

static I32 ll_regex_add_to_buffer(const char* lex_text, void* lex_val, U32 len, Stack* lexer_state)
{
    (void) lex_val;
    (void) lexer_state;

    if (len + regex_buffer.n >= regex_buffer.s)
    {
        regex_buffer.s *= 2;
        regex_buffer.buffer = realloc(regex_buffer.buffer, regex_buffer.s);
    }

    strncpy(regex_buffer.buffer + regex_buffer.n, lex_text, len);
    regex_buffer.n += len;
    return -1;
}

static LexerRule  ll_rules_regex[] = {
        {.regex_raw = "\"", .expr = (lexer_expr) ll_regex_quote},
        {.regex_raw = "[^\"]+", .expr = ll_regex_add_to_buffer}
};

static LexerRule* ll_rules[] = {
        ll_rules_s0,
        ll_rules_lex,
        ll_rules_grammar,
        ll_rules_match_brace,
        ll_rules_regex,
        ll_rules_header,
};

static U32 ll_rules_n[] = {
        ARR_LEN(ll_rules_s0),
        ARR_LEN(ll_rules_lex),
        ARR_LEN(ll_rules_grammar),
        ARR_LEN(ll_rules_match_brace),
        ARR_LEN(ll_rules_regex),
        ARR_LEN(ll_rules_header),
};

const U32 grammars[][7] = {
        /* Augmented rule */
        {TOK_GG_FILE},

        /* TOK_GG_KEY_VALS => */
        {TOK_OPTION},
        {TOK_GG_KEY_VALS, TOK_OPTION},

        // TOK_GG_HEADER =>
        {TOK_HEADER, TOK_GG_KEY_VALS},
        {TOK_GG_KEY_VALS},

        /* Lexer Rules */
        {TOK_REGEX_RULE, TOK_G_ACTION},
        {TOK_GG_LEX_RULE},
        {TOK_GG_LEX_RULES, TOK_GG_LEX_RULE},

        /* Grammar rules */
        // TOK_GG_TOKENS =>
        {TOK_G_TOK},
        {TOK_GG_TOKENS, TOK_G_TOK},

        // TOK_GG_SINGLE_GRAMMAR =>
        {TOK_GG_TOKENS, TOK_G_ACTION},

        // TOK_GG_MULTI_GRAMMAR =>
        {TOK_GG_SINGLE_GRAMMAR},
        {TOK_GG_MULTI_GRAMMAR, TOK_G_OR, TOK_GG_SINGLE_GRAMMAR},

        // TOK_GG_GRAMMAR =>
        {TOK_G_EXPR_DEF, TOK_GG_MULTI_GRAMMAR, TOK_G_TERM},

        // TOK_GG_GRAMMARS =>
        {TOK_GG_GRAMMAR},
        {TOK_GG_GRAMMARS, TOK_GG_GRAMMAR},

        // TOK_GG_FILE =>
        {TOK_GG_HEADER, TOK_DELIMITER, TOK_GG_LEX_RULES, TOK_DELIMITER, TOK_DELIMITER, TOK_GG_GRAMMARS, TOK_DELIMITER}
};

static void gg_key_val_add_next(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = args[0].key_val;
    dest->key_val->next = args[1].key_val;
}
static void gg_build_header_1(CodegenUnion* dest, CodegenUnion* args)
{
    dest->key_val = args[0].key_val;
    dest->key_val->next = args[1].key_val;
}
static void gg_build_l_rule_1(CodegenUnion* dest, CodegenUnion* args)
{
    dest->l_rule = malloc(sizeof(struct LexerRuleProto));
    dest->l_rule->regex = args[0].string;
    dest->l_rule->function = args[1].string;
    dest->l_rule->next = NULL;
}
static void gg_build_l_rule_2(CodegenUnion* dest, CodegenUnion* args)
{
    dest->l_rule = args[0].l_rule;
    dest->l_rule->next = args[1].l_rule;
}
static void gg_build_tok_1(CodegenUnion* dest, CodegenUnion* args)
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
        {.token=TOK_GG_KEY_VALS, .tok_n=2, .grammar=grammars[2],       .expr = (parser_expr) gg_key_val_add_next},
        {.token=TOK_GG_HEADER, .tok_n=2, .grammar=grammars[3],         .expr = (parser_expr) gg_build_header_1},
        {.token=TOK_GG_HEADER, .tok_n=1, .grammar=grammars[4]},
        {.token=TOK_GG_LEX_RULE, .tok_n=2, .grammar=grammars[5],       .expr = (parser_expr) gg_build_l_rule_1},
        {.token=TOK_GG_LEX_RULES, .tok_n=1, .grammar=grammars[6]},
        {.token=TOK_GG_LEX_RULES, .tok_n=2, .grammar=grammars[7],      .expr = (parser_expr) gg_build_l_rule_2},
        {.token=TOK_GG_TOKENS, .tok_n=1, .grammar=grammars[8]},
        {.token=TOK_GG_TOKENS, .tok_n=2, .grammar=grammars[9],         .expr = (parser_expr) gg_build_tok_1},
        {.token=TOK_GG_SINGLE_GRAMMAR, .tok_n=2, .grammar=grammars[10], .expr = (parser_expr) gg_build_single},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=1, .grammar=grammars[11]},
        {.token=TOK_GG_MULTI_GRAMMAR, .tok_n=3, .grammar=grammars[12], .expr = (parser_expr) gg_build_multi},
        {.token=TOK_GG_GRAMMAR, .tok_n=3, .grammar=grammars[13],       .expr = (parser_expr) gg_build_grammar},
        {.token=TOK_GG_GRAMMARS, .tok_n=1, .grammar=grammars[14]},
        {.token=TOK_GG_GRAMMARS, .tok_n=2, .grammar=grammars[15],      .expr = (parser_expr) gg_build_grammars},
        {.token=TOK_GG_FILE, .tok_n=7, .grammar=grammars[16],          .expr = (parser_expr) gg_build_file},
};

U8 precedence_table[TOK_AUGMENT] = {0};

int gen_parser_init(GrammarParser* self)
{
    self->lexer_rules = ll_rules;
    self->grammar_rules = gg_rules;
    self->lex_state_n = ARR_LEN(ll_rules_n);
    self->lex_n = ll_rules_n;
    self->grammar_n = ARR_LEN(gg_rules);
    self->action_token_n = TOK_GG_FILE;
    self->token_n = TOK_AUGMENT;

    U32 regex_init_errors;
    if ((regex_init_errors = parser_init(self)))
    {
        fprintf(stderr,"Failed to initialize %d regular expressions\n", regex_init_errors);
        return 1;
    }

    // Generate the parsing table
    CanonicalCollection* cc = canonical_collection_init(self);
    canonical_collection_resolve(cc, LALR_1);

    GEN_parsing_table = canonical_collection_generate(cc, precedence_table);
    dump_table(GEN_parsing_table, cc, tok_names, 0);
    canonical_collection_free(cc);

    return 0;
}
