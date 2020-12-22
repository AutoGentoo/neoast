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

typedef int (*lexer_expr) (const char* lex_text, void* lex_val, char* skip);

struct LexerRule_prv
{
    regex_t regex;
    lexer_expr expr;
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
int lex_next(const char** input, GrammarParser* parser, void* lval);

#endif //NEOAST_LEXER_H
