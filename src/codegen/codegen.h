//
// Created by tumbar on 12/25/20.
//

#ifndef NEOAST_CODEGEN_H
#define NEOAST_CODEGEN_H

#include <common.h>

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
} key_val_t;

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = 1,
    TOK_HEADER, // 2
    TOK_UNION, // 3
    TOK_DELIMITER, // 4

    /* Lexer tokens */
    TOK_LEX_STATE, // 5
    TOK_REGEX_RULE, // 6

    /* Grammar tokens */
    TOK_G_EXPR_DEF, // 7
    TOK_G_TOK, // 8
    TOK_G_OR, // 9
    TOK_G_TERM, // 10
    TOK_G_ACTION, // 11

    /* Grammar rules */
    TOK_GG_FILE, // 12

    /* Header grammar */
    TOK_GG_KEY_VALS, // 13
    TOK_GG_HEADER, // 14

    /* Lexing grammar */
    TOK_GG_LEX_RULE, // 15
    TOK_GG_LEX_RULES, // 16

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS, // 17
    TOK_GG_GRAMMAR, // 18
    TOK_GG_TOKENS, // 19
    TOK_GG_SINGLE_GRAMMAR, // 20
    TOK_GG_MULTI_GRAMMAR, // 21

    /* Artificial grammar */
    TOK_AUGMENT, // 22
};

enum
{
    LEX_STATE_LEXER_RULES = 1,
    LEX_STATE_GRAMMAR_RULES,
    LEX_STATE_MATCH_BRACE,
    LEX_STATE_REGEX,
    LEX_STATE_LEX_STATE,
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

extern U32* GEN_parsing_table;
extern const char* tok_names_errors[];

#endif //NEOAST_CODEGEN_H
