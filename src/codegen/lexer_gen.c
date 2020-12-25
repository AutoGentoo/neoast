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

U32* GEN_lexer_table = NULL;

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = 1,
    TOK_HEADER,
    TOK_BRACKET_STMT,
    TOK_DELIMITER,
    TOK_REGEX_MACRO,
    TOK_REGEX_RULE,
    TOK_LL_FILE,
    TOK_LL_OPTIONS,
    TOK_LL_LEX_RULE,
    TOK_LL_LEX_RULES,
    TOK_LL_MACROS,
    TOK_AUGMENT,
};

static const
char* tok_names = "$OHBDMXFR.*A";

static I32 ll_option(const char* lex_text, LexerUnion* lex_val)
{
    // Skip over '%option '
    const char* text_skip = lex_text + 8;
    const char* divider = strchr(text_skip, '=');

    lex_val->option.option_name = strndup(text_skip, text_skip - divider);
    lex_val->option.value = strdup(divider + 1);
    lex_val->option.next = NULL;

    return TOK_OPTION;
}

static I32
ll_header(const char* lex_text, LexerUnion* lex_val, U32 len)
{
    lex_val->string = malloc(len - 3);
    memcpy(lex_val->string, lex_text + 2, len - 4);
    lex_val->string[len - 4] = 0;
    return TOK_HEADER;
}

static I32
ll_braces(const char* lex_text, LexerUnion* lex_val, U32 len)
{
    lex_val->string = malloc(len - 1);
    memcpy(lex_val->string, lex_text + 1, len - 2);
    lex_val->string[len - 2] = 0;
    return TOK_BRACKET_STMT;
}

static I32
ll_regex(const char* lex_text, LexerUnion* lex_val, U32 len)
{
    printf("Found regex %s\n", lex_text);
    lex_val->string = malloc(len + 1);
    memcpy(lex_val->string, lex_text, len);
    lex_val->string[len] = 0;
    return TOK_REGEX_RULE;
}

static I32
ll_macro(const char* lex_text, LexerUnion* lex_val, U32 len)
{
    printf("Found macro %s\n", lex_text);
    lex_val->string = malloc(len + 1);
    memcpy(lex_val->string, lex_text, len);
    lex_val->string[len] = 0;
    return TOK_REGEX_MACRO;
}

static LexerRule ll_rules[] = {
        {.regex_raw = "^[\n ]+"}, // skip
        {.regex_raw = "%%", .tok = TOK_DELIMITER},
        {.regex_raw = "^%option [A-z][A-z|0-9]*=\"[A-z|0-9_\\-\\+]*\"", .expr = (lexer_expr) ll_option},
        {.regex_raw = "^%{(.*[\n])+%}", .expr = (lexer_expr) ll_header},
        {.regex_raw = "^{(?:[^}{]+|(?R))*+}", .expr = (lexer_expr) ll_braces},
        {.regex_raw = "^[A-z_]+[\\s]+(\\[(?:[^\\]\\[]+|(?R))*+\\][^\\s]*)+", .expr = (lexer_expr) ll_macro},
        {.regex_raw = "^(\\[(?:[^\\]\\[]+|(?R))*+\\][^\\s]*)+", .expr = (lexer_expr) ll_regex}
};

U32 grammars[][5] = {
        /* Augmented rule */
        {TOK_LL_FILE},

        /* LL_MACROS */
        {TOK_REGEX_MACRO},
        {TOK_LL_MACROS, TOK_REGEX_MACRO},

        /* LL_OPTIONS -> */
        {TOK_OPTION},
        {TOK_LL_OPTIONS, TOK_OPTION},

        /* TOK_LL_LEX_RULE -> */
        {TOK_BRACKET_STMT, TOK_BRACKET_STMT},
        {TOK_REGEX_RULE, TOK_BRACKET_STMT},

        /* Rules */
        {TOK_LL_LEX_RULE},
        {TOK_LL_LEX_RULES, TOK_LL_LEX_RULE},

        /* File */
        {TOK_DELIMITER, TOK_LL_LEX_RULES, TOK_DELIMITER},
        {TOK_LL_OPTIONS, TOK_DELIMITER, TOK_LL_LEX_RULES, TOK_DELIMITER},
        {TOK_HEADER, TOK_DELIMITER, TOK_LL_LEX_RULES, TOK_DELIMITER},
        {TOK_LL_OPTIONS, TOK_HEADER, TOK_DELIMITER, TOK_LL_LEX_RULES, TOK_DELIMITER},
};

static void
gg_copy(LexerUnion* dest, LexerUnion* args)
{
    *dest = args[0];
}

static void
gg_option_add_next(LexerUnion* dest, LexerUnion* args)
{
    // args => {TOK_LL_OPTIONS, TOK_OPTION}
    dest->option = args[0].option;

    struct Option* next = malloc(sizeof(struct Option));
    next->next = NULL;
    next->value = args[1].option.value;
    next->option_name = args[1].option.option_name;
}

static void
gg_lex_rule_bracket_build(LexerUnion* dest, LexerUnion* args)
{
    // args => {TOK_BRACKET_STMT, TOK_BRACKET_STMT}

}

static GrammarRule gg_rules[] = {
        {.token=TOK_AUGMENT, .tok_n=1, .grammar=grammars[0],    (parser_expr) gg_copy},
        {.token=TOK_LL_MACROS, .tok_n=1, .grammar=grammars[1], (parser_expr) gg_copy},
        {.token=TOK_LL_MACROS, .tok_n=2, .grammar=grammars[2]}, // TODO
        {.token=TOK_LL_OPTIONS, .tok_n=1, .grammar=grammars[3], (parser_expr) gg_copy},
        {.token=TOK_LL_OPTIONS, .tok_n=2, .grammar=grammars[4], (parser_expr) gg_option_add_next},
        {.token=TOK_LL_LEX_RULE, .tok_n=2, .grammar=grammars[5]},
        {.token=TOK_LL_LEX_RULE, .tok_n=2, .grammar=grammars[6]},
        {.token=TOK_LL_LEX_RULES, .tok_n=1, .grammar=grammars[7]},
        {.token=TOK_LL_LEX_RULES, .tok_n=2, .grammar=grammars[8]},
        {.token=TOK_LL_FILE, .tok_n=3, .grammar=grammars[9]},
        {.token=TOK_LL_FILE, .tok_n=4, .grammar=grammars[10]},
        {.token=TOK_LL_FILE, .tok_n=4, .grammar=grammars[11]},
        {.token=TOK_LL_FILE, .tok_n=5, .grammar=grammars[12]},
};

U8 precedence_table[TOK_AUGMENT] = {0};

void gen_parser_init(GrammarParser* self)
{
    self->lexer_rules = ll_rules;
    self->grammar_rules = gg_rules;
    self->lex_n = ARR_LEN(ll_rules);
    self->grammar_n = ARR_LEN(gg_rules);
    self->action_token_n = TOK_LL_FILE;
    self->token_n = TOK_AUGMENT;

    parser_init(self);

    // Generate the parsing table
    CanonicalCollection* cc = canonical_collection_init(self);
    canonical_collection_resolve(cc, lalr_1_cmp, lalr_1_merge);

    GEN_lexer_table = canonical_collection_generate(cc, precedence_table);
    dump_table(GEN_lexer_table, cc, tok_names, 0);
    canonical_collection_free(cc);
}
