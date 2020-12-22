//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H

#include "common.h"

typedef void* (*parser_expr) (void** expr_tokens);

struct GrammarParser_prv
{
    LL(LexerRule*) lexer_rules;
    LL(GrammarRule*) grammar_rules;
};

struct GrammarRule
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

#endif //NEOAST_PARSER_H
