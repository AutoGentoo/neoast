//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H

#include "common.h"

// Tokens lexified at one time
#define NEOAST_TOKEN_T_N 64

typedef void* (*parser_expr) (void** expr_tokens);

struct GrammarParser_prv
{
    int lex_n;
    int grammar_n;
    LexerRule* lexer_rules;
    GrammarRule* grammar_rules;
};

struct GrammarRule_prv
{
    LL(GrammarSubRule*) expressions;
    GrammarRule* next;
};

struct GrammarSubRule_prv
{
    GrammarSubRule* next;
    parser_expr expr;
    int tokens[];
};

struct ParserState_prv
{
    const char* buffer;
    void* lval;
};

/**
 * Use the lexer to get every token in a string
 * We will stop parsing on the first token that is
 * not handled by a lexer rule
 * @param input buffer to parse
 * @param parse grammar parser reference
 * @param table table of tokens
 * @param val_table
 * @param val_n
 */
void parser_fill_table(const char** input,
                       GrammarParser* parse,
                       int* table,
                       void* val_table,
                       size_t val_n);

//int parser_reduce(void* val_table, )

#endif //NEOAST_PARSER_H
