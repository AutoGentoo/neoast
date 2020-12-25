//
// Created by tumbar on 12/25/20.
//

#include <regex.h>
#include "lexer.h"
#include "parser.h"

void parser_init(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_n; i++)
    {
        regcomp(&self->lexer_rules[i].regex, self->lexer_rules[i].regex_raw, REG_EXTENDED);
    }
}

void parser_free(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_n; i++)
    {
        regfree(&self->lexer_rules[i].regex);
    }
}