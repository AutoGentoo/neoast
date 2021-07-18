#ifndef NEOAST_GRAMMAR_H
#define NEOAST_GRAMMAR_H

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
    TOK_END_STATE, // 9

    /* Grammar tokens */
    TOK_G_EXPR_DEF, // 10
    TOK_G_TOK, // 11
    TOK_G_OR, // 12
    TOK_G_TERM, // 13
    TOK_G_ACTION, // 14

    /* Grammar rules */
    TOK_GG_FILE, // 15

    /* Header grammar */
    TOK_GG_KEY_VALS, // 16
    TOK_GG_HEADER, // 17

    /* Lexing grammar */
    TOK_GG_LEX_RULE, // 18
    TOK_GG_LEX_RULES, // 19

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS, // 20
    TOK_GG_GRAMMAR, // 21
    TOK_GG_TOKENS, // 22
    TOK_GG_SINGLE_GRAMMAR, // 23
    TOK_GG_MULTI_GRAMMAR, // 24

    /* Artificial grammar */
    TOK_AUGMENT, // 25
};

NEOAST_COMPILE_ASSERT(TOK_AUGMENT == 25, assert_token_length);

enum
{
    LEX_STATE_LEXER_RULES = 1,
    LEX_STATE_GRAMMAR_RULES,
    LEX_STATE_MATCH_BRACE,
    LEX_STATE_REGEX,
    LEX_STATE_LEX_STATE,
    LEX_STATE_COMMENT
};

typedef union {
    KeyVal* key_val;
    struct LexerRuleProto* l_rule;
    struct Token* token;
    struct GrammarRuleSingleProto* g_single_rule;
    struct GrammarRuleProto* g_rule;
    struct File* file;
    char* string;
} CodegenUnion;

typedef union {
    CodegenUnion value;
    TokenPosition position;
} CodegenStruct;

#endif //NEOAST_GRAMMAR_H
