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
#include <parsergen/canonical_collection.h>

#define CODEGEN_STRUCT "NeoastValue"
#define CODEGEN_UNION "NeoastUnion"

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

int gen_parser_init(GrammarParser* self);
int codegen_write(const char* grammar_file_path, const struct File* self, FILE* fp);

void emit_warning(const TokenPosition* position, const char* message, ...);
void emit_error(const TokenPosition* position, const char* message, ...);
int has_errors();

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

#endif //NEOAST_CODEGEN_H
