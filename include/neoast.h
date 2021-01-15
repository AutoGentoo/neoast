#ifndef NEOAST_H
#define NEOAST_H

#include <cre2.h>

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
    LexerRule* const* lexer_rules;
    const GrammarRule* grammar_rules;
    const char* const* token_names;

    // Also number of columns
    U32 token_n;
    U32 action_token_n;
};

#ifndef NEOAST_PARSER_H
U32 parser_init(void*);
void parser_free(void*);
void* parser_allocate_buffers(int, int, int, size_t);
void parser_free_buffers(void*);
void parser_reset_buffers(const void* self);

U32 lexer_fill_table(const char*, U64, const void*, const void*);
I32 parser_parse_lr(const void*, const void*, const U32*);
#endif

#endif