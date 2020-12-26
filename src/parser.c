//
// Created by tumbar on 12/25/20.
//

#include <regex.h>
#include "lexer.h"
#include "parser.h"

void parser_init(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_state_n; i++)
    {
        for (U32 j = 0; j < self->lex_n[i]; j++)
        {
            regcomp(&self->lexer_rules[i][j].regex, self->lexer_rules[i][j].regex_raw, REG_EXTENDED);
        }
    }
}

void parser_free(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_state_n; i++)
    {
        for (U32 j = 0; j < self->lex_n[i]; j++)
        {
            regfree(&self->lexer_rules[i][j].regex);
        }
    }
}