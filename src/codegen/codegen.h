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

#ifdef __cplusplus
extern "C" {
#endif

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
    KEY_VAL_LEXER,
    KEY_VAL_INCLUDE,
} key_val_t;

typedef struct KeyVal_ KeyVal;

struct KeyVal_
{
    TokenPosition position;
    key_val_t type;
    char* key;
    char* value;

    KeyVal* back;
    KeyVal* next;
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
    KeyVal* header;
    struct LexerRuleProto* lexer_rules;
    struct GrammarRuleProto* grammar_rules;
};

int gen_parser_init(GrammarParser* self, void** lexer_ptr);
int codegen_write(const char* grammar_file_path,
                  const struct File* self,
                  const char* output_file_cc,
                  const char* output_file_hh);

void emit_warning(const TokenPosition* position, const char* message, ...);
void emit_error(const TokenPosition* position, const char* message, ...);
uint32_t has_errors();

void lexing_error_cb(void* error_ctx,
                     const char* input,
                     const TokenPosition* position,
                     const char* lexer_state);

void parsing_error_cb(void* error_ctx,
                      const char* const* token_names,
                      const TokenPosition* position,
                      uint32_t last_token,
                      uint32_t current_token,
                      const uint32_t expected_tokens[],
                      uint32_t expected_tokens_n);

KeyVal* key_val_build(const TokenPosition* p, key_val_t type, char* key, char* value);
struct Token* build_token(const TokenPosition* position, char* name);
struct Token* build_token_ascii(const TokenPosition* position, char value);
char* get_ascii_token_name(char value);
char get_ascii_from_name(const char* name);

void tokens_free(struct Token* self);
void key_val_free(KeyVal* self);
void lexer_rule_free(struct LexerRuleProto* self);
void grammar_rule_single_free(struct GrammarRuleSingleProto* self);
void grammar_rule_multi_free(struct GrammarRuleProto* self);
void file_free(struct File* self);

extern const char* tok_names_errors[];

#ifdef __cplusplus
}
#endif

#endif //NEOAST_CODEGEN_H
