//
// Created by tumbar on 12/25/20.
//

#include <regex.h>
#include "lexer.h"
#include "parser.h"

U32 parser_init(GrammarParser* self)
{
    U32 error = 0;
    for (U32 i = 0; i < self->lex_state_n; i++)
    {
        for (U32 j = 0; j < self->lex_n[i]; j++)
        {
            if (regcomp(&self->lexer_rules[i][j].regex, self->lexer_rules[i][j].regex_raw, REG_EXTENDED) != 0)
            {
                error++;
                fprintf(stderr, "Failed to compile regular expression '%s'\n", self->lexer_rules[i][j].regex_raw);
            }
        }
    }

    return error;
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