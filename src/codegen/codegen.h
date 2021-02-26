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


#ifndef NEOAST_CODEGEN_H
#define NEOAST_CODEGEN_H

#include <neoast.h>

typedef enum
{
    KEY_VAL_HEADER, // Key not filled
    KEY_VAL_OPTION,
    KEY_VAL_TOKEN,
    KEY_VAL_TOKEN_ASCII,
    KEY_VAL_TOKEN_TYPE,
    KEY_VAL_TOKEN_TYPE_ASCII,
    KEY_VAL_TYPE,
    KEY_VAL_LEFT,
    KEY_VAL_RIGHT,
    KEY_VAL_MACRO,
    KEY_VAL_START,
    KEY_VAL_STATE,
    KEY_VAL_UNION,
    KEY_VAL_BOTTOM,
    KEY_VAL_DESTRUCTOR,
} key_val_t;

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = 1,
    TOK_HEADER, // 2
    TOK_BOTTOM, // 3
    TOK_UNION, // 4
    TOK_DESTRUCTOR, // 5
    TOK_DELIMITER, // 6

    /* Lexer tokens */
    TOK_LEX_STATE, // 7
    TOK_REGEX_RULE, // 8

    /* Grammar tokens */
    TOK_G_EXPR_DEF, // 9
    TOK_G_TOK, // 10
    TOK_G_OR, // 11
    TOK_G_TERM, // 12
    TOK_G_ACTION, // 13

    /* Grammar rules */
    TOK_GG_FILE, // 14

    /* Header grammar */
    TOK_GG_KEY_VALS, // 15
    TOK_GG_HEADER, // 16

    /* Lexing grammar */
    TOK_GG_LEX_RULE, // 17
    TOK_GG_LEX_RULES, // 18

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS, // 19
    TOK_GG_GRAMMAR, // 20
    TOK_GG_TOKENS, // 21
    TOK_GG_SINGLE_GRAMMAR, // 22
    TOK_GG_MULTI_GRAMMAR, // 23

    /* Artificial grammar */
    TOK_AUGMENT, // 24
};

NEOAST_COMPILE_ASSERT(TOK_AUGMENT == 24, assert_token_length);

enum
{
    LEX_STATE_LEXER_RULES = 1,
    LEX_STATE_GRAMMAR_RULES,
    LEX_STATE_MATCH_BRACE,
    LEX_STATE_REGEX,
    LEX_STATE_LEX_STATE,
    LEX_STATE_COMMENT
};

struct KeyVal
{
    key_val_t type;
    char* key;
    char* value;
    uint64_t options;
    struct KeyVal* next;
};

struct LexerRuleProto
{
    char* lexer_state; // NULL for default
    char* regex;
    char* function;
    struct LexerRuleProto* next;
};

struct Token
{
    char* name;
    struct Token* next;
};

struct GrammarRuleSingleProto
{
    struct Token* tokens;
    char* function;
    struct GrammarRuleSingleProto* next;
};

struct GrammarRuleProto
{
    char* name;
    struct GrammarRuleSingleProto* rules;
    struct GrammarRuleProto* next;
};

struct File
{
    struct KeyVal* header;
    struct LexerRuleProto* lexer_rules;
    struct GrammarRuleProto* grammar_rules;
};

typedef union {
    struct KeyVal* key_val;
    struct LexerRuleProto* l_rule;
    struct Token* token;
    struct GrammarRuleSingleProto* g_single_rule;
    struct GrammarRuleProto* g_rule;
    struct File* file;
    char* string;
} CodegenUnion;

int gen_parser_init(GrammarParser* self);
int codegen_write(const struct File* self, FILE* fp);

void file_free(struct File* self);

extern uint32_t* GEN_parsing_table;
extern const char* tok_names_errors[];

#endif //NEOAST_CODEGEN_H
