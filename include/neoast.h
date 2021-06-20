/* 
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEOAST_H
#define NEOAST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stdio.h>

#define NEOAST_ASCII_MAX ('~' + 1)
#define NEOAST_ARR_LEN(arr) ((sizeof(arr)) / sizeof(((arr)[0])))
#define NEOAST_STACK_PUSH(stack, i) (stack)->data[((stack)->pos)++] = (i)
#define NEOAST_STACK_POP(stack) (stack)->data[--((stack)->pos)]
#define NEOAST_STACK_PEEK(stack) (stack)->data[(stack)->pos - 1]

#define NEOAST_COMPILE_ASSERT(assertion, name) extern int name ## _gbl_assertion_compile___[(assertion) ? 1 : -1]

typedef struct GrammarParser_prv GrammarParser;
typedef struct GrammarRule_prv GrammarRule;
typedef struct ParsingStack_prv ParsingStack;
typedef struct ParserBuffers_prv ParserBuffers;
typedef struct TokenPosition_prv TokenPosition;

// Codegen
typedef struct LR_1_prv LR_1;
typedef struct GrammarState_prv GrammarState;
typedef struct CanonicalCollection_prv CanonicalCollection;

typedef void (*parser_expr) (void* dest, void** values);
typedef int32_t (*lexer_expr) (
        const char* lex_text,
        void* lex_val,
        uint32_t len,
        ParsingStack* ll_state,
        TokenPosition* position);
typedef void (*parser_destructor) (void* self);


// User defined error callbacks

// Called when no matching token could be found
typedef void (*ll_error_cb)(
        const char* input,               //!< Input passed in with parse()
        const TokenPosition* position);  //!< Position of unmatched token (or NULL if track_position != TRUE)

typedef void (*yy_error_cb)(
        const char* const* token_names,
        const TokenPosition* position,
        uint32_t last_token,
        uint32_t current_token,
        const uint32_t expected_tokens[],
        uint32_t expected_tokens_n);

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
    LEXER_OPT_NONE = 0,
    LEXER_OPT_LONGEST_MATCH = 1 << 0,
    LEXER_OPT_TOKEN_POS = 1 << 1,
} lexer_option_t;

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
    const uint32_t* ascii_mappings;
    const GrammarRule* grammar_rules;
    const char* const* token_names;
    parser_destructor const* destructors;

    void* lexer_ptr;
    void (*lexer_free)(void*);
    yy_error_cb parser_error;

    // Also number of columns
    uint32_t grammar_n;
    uint32_t token_n;
    uint32_t action_token_n;
    lexer_option_t lexer_opts;
};

struct ParsingStack_prv
{
    uint32_t pos;
    uint32_t data[];
};

typedef struct
{
    uint32_t line;
    uint32_t line_start_offset;
} PositionData;

struct ParserBuffers_prv
{
    void* value_table;                  //!< Value table
    int32_t* token_table;               //!< Token table
    ParsingStack* parsing_stack;        //!< LR parsing stack
    uint32_t max_token_length;          //!< Size of buffer for a token
    uint32_t val_s;                     //!< Size of each value in bytes
    uint32_t union_s;                   //!< If no token position data, this is the same as val_s
    uint32_t table_n;                   //!< Number of tokens/values in the tables
};

struct TokenPosition_prv
{
    uint32_t line;
    uint32_t col_start;
};

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H
uint32_t parser_init(GrammarParser* self);
void parser_free(GrammarParser* self);

ParsingStack* parser_allocate_stack(size_t stack_n);
void parser_free_stack(ParsingStack* self);
ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len,
                                       int parsing_stack_n,
                                       size_t val_s,
                                       size_t union_s);
void parser_free_buffers(ParserBuffers* self);
void parser_reset_buffers(const ParserBuffers* self);

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
int32_t parser_parse_lr(const GrammarParser* parser,
                        const uint32_t* parsing_table,
                        const ParserBuffers* buffers,
                        void* lexer,
                        int ll_next(void*, void*));
#endif

#ifdef __cplusplus
};
#endif

#endif