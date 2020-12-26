//
// Created by tumbar on 12/22/20.
//

#include <stdlib.h>
#include "string.h"
#include "parser.h"
#include "lexer.h"

static __thread char lexdest[NEOAST_MAX_TOK_N];

U32 lexer_fill_table(const char** input, const GrammarParser* parse, U32* table, void* val_table, U32 val_n, U32 table_n)
{
    void* current_val = val_table;
    int i = 0;
    U32 lex_state = 0;
    while ((table[i] = lex_next(input, parse, current_val, &lex_state)) && i < table_n) // While not EOF
    {
        if (table[i] == -1)
        {
            fprintf(stderr, "Invalid character '%c'\n", *((*input) - 1));
        }

        current_val += val_n;
        i++;
    }

    return i;
}

int lex_next(const char** input, const GrammarParser* parser, void* lval, U32* lex_state)
{
    if (!**input)
        return 0;

    regmatch_t regex_matches[1];
    LexerRule* rule;
    LexerRule* state;

    int i = 0;
    while(i < parser->lex_n[*lex_state])
    {
        state = parser->lexer_rules[*lex_state];
        rule = &state[i++];
        if (regexec(&rule->regex, *input, 1, regex_matches, 0) == 0)
        {
            // Match found
            // Build the destination
            size_t n = regex_matches[0].rm_eo - regex_matches[0].rm_so;
            I32 token;

            if (rule->expr)
            {
                if (n > NEOAST_MAX_TOK_N)
                {
                    char* dest = strndup(*input + regex_matches[0].rm_so, n);
                    token = rule->expr(dest, lval, n, lex_state);
                    free(dest);
                }
                else
                {
                    strncpy(lexdest, *input + regex_matches[0].rm_so, n);
                    token = rule->expr(lexdest, lval, n, lex_state);
                }
            }
            else if (rule->tok)
            {
                token = rule->tok;
            }
            else
            {
                token = -1; // skip
            }

            *input += regex_matches[0].rm_eo;
            if (token > 0)
                return token;

            i = 0;
        }
    }

    // Invalid token
    if (**input)
    {
        (*input)++;
        return -1;
    }

    return 0;
}