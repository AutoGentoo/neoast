//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_LEXER_H
#define NEOAST_LEXER_H

#include <stdio.h>
#include <regex.h>
#include "common.h"

// Length of the token buffer
// Anything longer will be dynamically allocated
#define NEOAST_MAX_TOK_N 1024

typedef I32 (*lexer_expr) (char* lex_text, void* lex_val, U32 len, U32* ll_state);

struct LexerRule_prv
{
    regex_t regex;
    lexer_expr expr;
    I32 tok;
    const char* regex_raw;
};

/**
 * Get the next token in our buffer
 * We want to use FILE* here even if we
 * feed in char* so that we can use the builtin
 * buffer in the FILE protocol
 * @param input input buffer
 * @param parser object that has the lexer rules
 * @param lval destination of the lexer rule
 * @return next token in buffer
 */
int lex_next(const char** input, const GrammarParser* parser, void* lval, U32* lex_state);

/**
 * Use the lexer to get every token in a string
 * We will stop parsing on the first token that is
 * not handled by a lexer rule
 * @param input buffer to parse
 * @param parse grammar parser reference
 * @param table table of tokens that we parsed (null terminated)
 * @param val_table value table
 * @param val_n offset of a value (size of value union)
 * @return number of parsed tokens
 */
U32 lexer_fill_table(const char** input, const GrammarParser* parse, U32* table, void* val_table, U32 val_n, U32 table_n);

#endif //NEOAST_LEXER_H
