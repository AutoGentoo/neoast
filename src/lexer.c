//
// Created by tumbar on 12/22/20.
//

#include "parser.h"
#include "lexer.h"

int lex_next(FILE* fp, GrammarParser* parser, char** dest)
{
    for (LexerRule* rule = parser->lexer_rules.head; rule; rule = rule->next)
    {
        // TODO Match regex in here
    }

    // No token found
    return 0;
}