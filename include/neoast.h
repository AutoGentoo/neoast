#ifndef NEOAST_H
#define NEOAST_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stdio.h>
#include <cre2.h>

#define NEOAST_ASCII_MAX ('~' + 1)
#define NEOAST_ARR_LEN(arr) ((sizeof(arr)) / sizeof(((arr)[0])))
#define NEOAST_STACK_PUSH(stack, i) (stack)->data[((stack)->pos)++] = (i)
#define NEOAST_STACK_POP(stack) (stack)->data[--((stack)->pos)]
#define NEOAST_STACK_PEEK(stack) (stack)->data[(stack)->pos - 1]

typedef struct LexerRule_prv LexerRule;
typedef struct GrammarParser_prv GrammarParser;
typedef struct GrammarRule_prv GrammarRule;
typedef struct ParsingStack_prv ParsingStack;
typedef struct ParserBuffers_prv ParserBuffers;

// Codegen
typedef struct LR_1_prv LR_1;
typedef struct GrammarState_prv GrammarState;
typedef struct CanonicalCollection_prv CanonicalCollection;

typedef void (*parser_expr) (void* dest, void** values);
typedef int32_t (*lexer_expr) (
        const char* lex_text,
        void* lex_val,
        uint32_t len,
        ParsingStack* ll_state);
typedef void (*parser_destructor) (void* self);

enum
{
    // Accept an input during an augmented state
    // step
    TOK_ACCEPT_MASK = 1 << 29,

    // Go to another parser state.
    // TOK & TOK_MASK == next state
    TOK_SHIFT_MASK = 1 << 30,

    // Points to a reduction that can be made
    // Passes the compound expression into a
    // parser grammar rule handler.
    TOK_REDUCE_MASK = 1 << 31,

    // Unexpected token or expression
    TOK_SYNTAX_ERROR = 0,

    // Tokens can have every other bit
    // enabled
    TOK_MASK = (0xFFFFFFFF) & ~(
            0
            | TOK_ACCEPT_MASK
            | TOK_SHIFT_MASK
            | TOK_REDUCE_MASK
    ),
};

enum
{
    PRECEDENCE_NONE,
    PRECEDENCE_LEFT,
    PRECEDENCE_RIGHT
};

struct GrammarRule_prv
{
    uint32_t token;
    uint32_t tok_n;
    const uint32_t* grammar;
    parser_expr expr;
};

struct GrammarParser_prv
{
    cre2_options_t* regex_opts;
    uint32_t grammar_n;
    uint32_t lex_state_n;
    const uint32_t* lex_n;
    const uint32_t* ascii_mappings;
    LexerRule* const* lexer_rules;
    const GrammarRule* grammar_rules;
    const char* const* token_names;
    parser_destructor const* destructors;

    // Also number of columns
    uint32_t token_n;
    uint32_t action_token_n;
};

struct ParsingStack_prv
{
    uint32_t pos;
    uint32_t data[];
};

struct ParserBuffers_prv
{
    void* value_table;
    uint32_t* token_table;
    ParsingStack* lexing_state_stack;
    ParsingStack* parsing_stack;
    char* text_buffer;
    uint32_t max_token_length;
    uint32_t val_s;
    uint32_t table_n;
};

struct LexerRule_prv
{
    cre2_regexp_t* regex;
    lexer_expr expr;
    int32_t tok;
    const char* regex_raw;
};

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H
uint32_t parser_init(GrammarParser* self);
void parser_free(GrammarParser* self);

ParsingStack* parser_allocate_stack(size_t stack_n);
void parser_free_stack(ParsingStack* self);
ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len, int max_lex_state_depth, size_t val_s);
void parser_free_buffers(ParserBuffers* self);
void parser_reset_buffers(const ParserBuffers* self);

int32_t parser_parse_lr(
        const GrammarParser* parser,
        const uint32_t* parsing_table,
        const ParserBuffers* buffers);
#endif

#ifndef NEOAST_LEXER_H
#define NEOAST_LEXER_H
int32_t lexer_fill_table(const char* input, size_t len, const GrammarParser* parse, const ParserBuffers* buf);

int
lex_next(const char* input, const GrammarParser* parser, const ParserBuffers* buf, void* lval, uint32_t len, uint32_t* offset);
#endif

#endif