#ifndef NEOAST_H
#define NEOAST_H

#include <stdio.h>
#include <cre2.h>

#define ASCII_MAX ('~' + 1)

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

typedef struct LexerRule_prv LexerRule;
typedef struct GrammarParser_prv GrammarParser;
typedef struct GrammarRule_prv GrammarRule;
typedef struct Stack_prv Stack;
typedef struct ParserBuffers_prv ParserBuffers;

// Codegen
typedef struct LR_1_prv LR_1;
typedef struct GrammarState_prv GrammarState;
typedef struct CanonicalCollection_prv CanonicalCollection;

typedef void (*parser_expr) (void* dest, void** values);

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
    U32 token;
    U32 tok_n;
    const U32* grammar;
    parser_expr expr;
};

struct GrammarParser_prv
{
    cre2_options_t* regex_opts;
    U32 grammar_n;
    U32 lex_state_n;
    const U32* lex_n;
    const U32* ascii_mappings;
    LexerRule* const* lexer_rules;
    const GrammarRule* grammar_rules;
    const char* const* token_names;

    // Also number of columns
    U32 token_n;
    U32 action_token_n;
};

struct Stack_prv
{
    U32 pos;
    U32 data[];
};

struct ParserBuffers_prv
{
    void* value_table;
    U32* token_table;
    Stack* lexing_state_stack;
    Stack* parsing_stack;
    char* text_buffer;
    U32 val_s;
    U32 table_n;
};

typedef I32 (*lexer_expr) (
        const char* lex_text,
        void* lex_val,
        U32 len,
        Stack* ll_state);

struct LexerRule_prv
{
    cre2_regexp_t* regex;
    lexer_expr expr;
    I32 tok;
    const char* regex_raw;
};

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H
U32 parser_init(GrammarParser* self);
void parser_free(GrammarParser* self);

Stack* parser_allocate_stack(U64 stack_n);
void parser_free_stack(Stack* self);
ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len, int max_lex_state_depth, size_t val_s);
void parser_free_buffers(ParserBuffers* self);
void parser_reset_buffers(const ParserBuffers* self);

I32 parser_parse_lr(
        const GrammarParser* parser,
        const U32* parsing_table,
        const ParserBuffers* buffers);
#endif

#ifndef NEOAST_LEXER_H
#define NEOAST_LEXER_H
I32 lexer_fill_table(const char* input, U64 len, const GrammarParser* parse, const ParserBuffers* buf);

int
lex_next(const char* input, const GrammarParser* parser, const ParserBuffers* buf, void* lval, U32 len, U32* offset);
#endif

#endif