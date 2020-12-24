//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H

#include <stdio.h>
#include "common.h"

#define OFFSET_VOID_PTR(ptr, s, i) (void*)(((char*)(ptr)) + ((s) * (i)))

typedef int (*parser_expr) (void* dest, void** values);

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
              TOK_ACCEPT_MASK
            | TOK_SHIFT_MASK
            | TOK_REDUCE_MASK
            ),
};

struct GrammarParser_prv
{
    U32 lex_n;
    U32 grammar_n;
    LexerRule* lexer_rules;
    GrammarRule* grammar_rules;

    U32 col_count;
    const U32* parser_table;
};

struct GrammarRule_prv
{
    U32 tok_n;
    U32* grammar;
    parser_expr expr;
};

struct ParserStack_prv
{
    U32 pos;
    U32 data[];
};

/**
 * Try to reduce the token table using
 * the grammar rules stored in parser.
 * Keep reducing the table until no more operations
 * can be performed.
 *
 * @param parser reference with grammar rules
 * @param n number of tokens/values (subject to changes after return)
 * @param tokens token data
 * @param val_table table of parsed values
 * @param val_s size of each value in val_table
 * @return
 */
void parser_parse_left_scan(const GrammarParser* parser, int* n, U32* tokens, void* val_table, U32 val_s);

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
        U32* token_table,
        void* val_table,
        size_t val_s);

#endif //NEOAST_PARSER_H
