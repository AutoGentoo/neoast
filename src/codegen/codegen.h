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
    KEY_VAL_TOP, // Key not filled
    KEY_VAL_BOTTOM,
    KEY_VAL_OPTION,
    KEY_VAL_TOKEN,
    KEY_VAL_TOKEN_ASCII,
    KEY_VAL_TOKEN_TYPE,
    KEY_VAL_TYPE,
    KEY_VAL_LEFT,
    KEY_VAL_RIGHT,
    KEY_VAL_MACRO,
    KEY_VAL_START,
    KEY_VAL_UNION,
    KEY_VAL_DESTRUCTOR,
} key_val_t;

struct KeyVal
{
    TokenPosition position;
    key_val_t type;
    char* key;
    char* value;
    struct KeyVal* next;
};

struct LexerRuleProto
{
    TokenPosition position;
    char* lexer_state; // NULL for default
    struct LexerRuleProto* state_rules;
    char* regex;
    char* function;
    struct LexerRuleProto* next;
};

struct Token
{
    TokenPosition position;
    char* name;
    char ascii;
    struct Token* next;
};

struct GrammarRuleSingleProto
{
    TokenPosition position;
    struct Token* tokens;
    char* function;
    struct GrammarRuleSingleProto* next;
};

struct GrammarRuleProto
{
    TokenPosition position;
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

int gen_parser_init(GrammarParser* self);
int codegen_write(const struct File* self, FILE* fp);

void emit_warning(const TokenPosition* position, const char* message, ...);
void emit_error(const TokenPosition* position, const char* message, ...);
int has_errors();

struct KeyVal* key_val_build(const TokenPosition* p, key_val_t type, char* key, char* value);
struct Token* build_token(const TokenPosition* position, char* name);
struct Token* build_token_ascii(const TokenPosition* position, char value);
char* get_ascii_token_name(char value);
char get_ascii_from_name(const char* name);

void tokens_free(struct Token* self);
void key_val_free(struct KeyVal* self);
void lexer_rule_free(struct LexerRuleProto* self);
void grammar_rule_single_free(struct GrammarRuleSingleProto* self);
void grammar_rule_multi_free(struct GrammarRuleProto* self);
void file_free(struct File* self);

extern const char* tok_names_errors[];

#endif //NEOAST_CODEGEN_H
