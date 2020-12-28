//
// Created by tumbar on 12/25/20.
//

#ifndef NEOAST_CODEGEN_H
#define NEOAST_CODEGEN_H

typedef enum
{
    KEY_VAL_HEADER, // Key not filled
    KEY_VAL_OPTION,
    KEY_VAL_TOKEN,
    KEY_VAL_TOKEN_TYPE,
    KEY_VAL_TYPE,
    KEY_VAL_LEFT,
    KEY_VAL_RIGHT,
    KEY_VAL_MACRO,
    KEY_VAL_START,
} key_val_t;

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = 1,
    TOK_HEADER, // 2
    TOK_DELIMITER, // 3

    /* Lexer tokens */
    TOK_REGEX_RULE, // 4

    /* Grammar tokens */
    TOK_G_EXPR_DEF, // 5
    TOK_G_TOK, // 6
    TOK_G_OR, // 7
    TOK_G_TERM, // 8
    TOK_G_ACTION, // 9

    /* Grammar rules */
    TOK_GG_FILE, // 10

    /* Header grammar */
    TOK_GG_KEY_VALS, // 11
    TOK_GG_HEADER, // 12

    /* Lexing grammar */
    TOK_GG_LEX_RULE, // 13
    TOK_GG_LEX_RULES, // 14

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS, // 15
    TOK_GG_GRAMMAR, // 16
    TOK_GG_TOKENS, // 17
    TOK_GG_SINGLE_GRAMMAR, // 18
    TOK_GG_MULTI_GRAMMAR, // 19

    /* Artificial grammar */
    TOK_AUGMENT, // 20
};

enum
{
    LEX_STATE_LEXER_RULES = 1,
    LEX_STATE_GRAMMAR_RULES,
    LEX_STATE_MATCH_BRACE,
    LEX_STATE_REGEX,
    LEX_STATE_HEADER,
    LEX_STATE_N
};

struct KeyVal
{
    key_val_t type;
    char* key;
    char* value;
    struct KeyVal* next;
};

struct LexerRuleProto
{
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

extern U32* GEN_parsing_table;
extern const char* tok_names_errors[];

#endif //NEOAST_CODEGEN_H
