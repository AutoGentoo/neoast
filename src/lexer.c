//
// Created by tumbar on 12/22/20.
//

#include <stdlib.h>
#include "string.h"
#include "parser.h"
#include "lexer.h"

static char lexdest[MAXIMUM_LEX_TOKEN_LEN];

int lex_next(const char** input, GrammarParser* parser, void* lval)
{
    regmatch_t regex_matches[1];
    LexerRule* rule;

    int i = 0;
    while(i < parser->lex_n)
    {
        rule = &parser->lexer_rules[i++];
        if (regexec(&rule->regex, *input, 1, regex_matches, 0) == 0)
        {
            // Match found
            // Build the destination
            int n = regex_matches[0].rm_eo - regex_matches[0].rm_so;
            int token;
            char skip = 0;
            if (n > MAXIMUM_LEX_TOKEN_LEN)
            {
                char* dest = strndup(*input + regex_matches[0].rm_so, n);
                token = rule->expr(dest, lval, &skip);
                free(dest);
            }
            else
            {
                strncpy(lexdest, *input + regex_matches[0].rm_so, n);
                token = rule->expr(lexdest, lval, &skip);
            }

            *input += regex_matches[0].rm_eo;
            if (!skip)
                return token;

            i = 0;
        }
    }

    // No token found
    *input += 1;
    return 0; // EOF token
}