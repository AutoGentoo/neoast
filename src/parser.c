//
// Created by tumbar on 12/25/20.
//

#include <regex.h>
#include "lexer.h"
#include "parser.h"

void parser_free(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_n; i++)
    {
        regfree(&self->lexer_rules[i].regex);
    }
}