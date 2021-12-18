#ifndef NEOAST_GRAMMAR_H
#define NEOAST_GRAMMAR_H

enum
{
    // -1 = skip, 0 = EOF
    TOK_OPTION = NEOAST_ASCII_MAX + 1, // 257
    TOK_TOP, // 2, 258
    TOK_BOTTOM, // 3, 259
    TOK_UNION, // 4, 260
    TOK_DESTRUCTOR, // 5, 261
    TOK_DELIMITER, // 6, 262

    /* Lexer tokens */
    TOK_LEX_STATE, // 7, 263
    TOK_REGEX_RULE, // 8, 264
    TOK_END_STATE, // 9, 265

    /* Grammar tokens */
    TOK_G_EXPR_DEF, // 10, 266
    TOK_G_TOK, // 11, 267
    TOK_G_OR, // 12, 268
    TOK_G_TERM, // 13, 269
    TOK_G_ACTION, // 14, 270

    /* Grammar rules */
    TOK_GG_FILE, // 15, 271

    /* Header grammar */
    TOK_GG_KEY_VALS, // 16, 272
    TOK_GG_HEADER, // 17, 273

    /* Lexing grammar */
    TOK_GG_LEX_RULE, // 18, 274
    TOK_GG_LEX_RULES, // 19, 275

    /* Grammar grammar :) */
    TOK_GG_GRAMMARS, // 20, 276
    TOK_GG_GRAMMAR, // 21, 277
    TOK_GG_TOKENS, // 22, 278
    TOK_GG_SINGLE_GRAMMAR, // 23, 279
    TOK_GG_MULTI_GRAMMAR, // 24, 280

    /* Artificial grammar */
    TOK_AUGMENT, // 25, 281
};

NEOAST_COMPILE_ASSERT(TOK_AUGMENT == NEOAST_ASCII_MAX + 25, assert_token_length);

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
