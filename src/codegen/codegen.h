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
    KEY_VAL_MACRO
} key_val_t;

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = 1,
    TOK_HEADER,
    TOK_DELIMITER,

    /* Lexer tokens */
    TOK_REGEX_RULE,

    /* Grammar tokens */
    TOK_G_EXPR_DEF,
    TOK_G_TOK,
    TOK_G_OR,
    TOK_G_TERM,
    TOK_G_ACTION,

    /* Grammar rules */
    TOK_GG_FILE,

    /* Header grammar */
    TOK_GG_KEY_VALS,
    TOK_GG_HEADER,

    /* Lexing grammar */
    TOK_GG_LEX_RULES,

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS,
    TOK_GG_GRAMMAR,
    TOK_GG_TOKENS,
    TOK_GG_SINGLE_GRAMMAR,
    TOK_GG_MULTI_GRAMMAR,

    /* Artificial grammar */
    TOK_AUGMENT,
};

enum
{
    LEX_STATE_DEFAULT,
    LEX_STATE_LEXER_RULES,
    LEX_STATE_GRAMMAR_RULES,
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

#endif //NEOAST_CODEGEN_H
