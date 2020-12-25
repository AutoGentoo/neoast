//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H

#include <stdio.h>
#include "common.h"

#define OFFSET_VOID_PTR(ptr, s, i) (void*)(((char*)(ptr)) + ((s) * (i)))

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

typedef enum
{
    PRECEDENCE_NONE,
    PRECEDENCE_REDUCE,
    PRECEDENCE_SHIFT
} precedence_t;

struct GrammarRule_prv
{
    U32 token;
    U32 tok_n;
    U32* grammar;
    parser_expr expr;
};

struct GrammarParser_prv
{
    U32 lex_n;
    U32 grammar_n;
    LexerRule* lexer_rules;
    GrammarRule* grammar_rules;

    // Also number of columns
    U32 token_n;
    U32 action_token_n;
};

struct ParserStack_prv
{
    U32 pos;
    U32 data[];
};

/**
 * Run the LR parsing algorithm
 * given a parser with the parsing
 * table filled.
 * @param parser target parser (kept constant)
 * @param stack used for parsing
 * @param token_table tokens from lexer
 * @param val_table values from lexer
 * @param val_s sizeof each value
 * @return index in token/value table where the parsed value resides
 */
I32 parser_parse_lr(
        const GrammarParser* parser,
        ParserStack* stack,
        const U32* parsing_table,
        U32* token_table,
        void* val_table,
        size_t val_s);

#endif //NEOAST_PARSER_H
