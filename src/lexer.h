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


#ifndef NEOAST_LEXER_H
#define NEOAST_LEXER_H

#include <stdio.h>
#include <cre2.h>
#include <neoast.h>

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
int32_t lex_next(const char* input, const GrammarParser* parser,
                 const ParserBuffers* buf, char* lval,
                 unsigned int len, unsigned int* offset);

/**
 * Use the lexer to get every token in a string
 * We will stop parsing on the first token that is
 * not handled by a lexer rule
 * @param input buffer to parse
 * @param parser grammar parser reference
 * @param table table of tokens that we parsed (null terminated)
 * @param val_table value table
 * @param val_n offset of a value (size of value union)
 * @return number of parsed tokens
 */
int32_t lexer_fill_table(const char* input, size_t len, const GrammarParser* parser,
                         const ParserBuffers* buf);

#endif //NEOAST_LEXER_H
