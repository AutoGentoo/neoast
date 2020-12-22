//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H

#include "common.h"

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

#endif //NEOAST_PARSER_H
