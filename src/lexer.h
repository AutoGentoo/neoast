//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_LEXER_H
#define NEOAST_LEXER_H

#include <stdio.h>
#include <regex.h>
#include "common.h"

typedef int (*lexer_expr) (const char* lex_text, void* lex_val, char* skip);

struct LexerRule_prv
{
    LexerRule* next;
    regex_t regex;
    lexer_expr expr;
};

/**
 * Get the next token in our buffer
 * We want to use FILE* here even if we
 * feed in char* so that we can use the builtin
 * buffer in the FILE protocol
 * @param fp input buffer
 * @param parser object that has the lexer rules
 * @param dest destination of the lexer text
 * @return next token in buffer
 */
int lex_next(FILE* fp, GrammarParser* parser, char** dest);

#endif //NEOAST_LEXER_H
