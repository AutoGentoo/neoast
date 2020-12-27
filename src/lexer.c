//
// Created by tumbar on 12/22/20.
//

#include "string.h"
#include "parser.h"
#include "lexer.h"

U32 lexer_fill_table(const char* input, const GrammarParser* parse, U32* table, void* val_table, U32 val_n, U32 table_n)
{
    void* current_val = val_table;
    int i = 0;
    U32 lex_state = 0;
    U32 length = strlen(input);
    U32 offset = 0;
    while ((table[i] = lex_next(input, parse, current_val, length, &offset, &lex_state)) && i < table_n) // While not EOF
    {
        if (table[i] == -1)
        {
            fprintf(stderr, "Invalid character '%c'\n", input[offset - 1]);
        }

        current_val += val_n;
        i++;
    }

    return i;
}

int lex_next(const char* input,
             const GrammarParser* parser,
             void* lval,
             U32 len,
             U32* offset,
             U32* lex_state)
{
    if (*offset >= len)
        return 0;

    cre2_string_t match;
    LexerRule* rule;
    LexerRule* state;

    int i = 0;
    while(i < parser->lex_n[*lex_state])
    {
        state = parser->lexer_rules[*lex_state];
        rule = &state[i++];

        if (cre2_match(rule->regex, input, len,
                        *offset, len, CRE2_ANCHOR_START,
                        &match, 1))
        {
            I32 token;

            if (rule->expr)
            {
                token = rule->expr(match.data, lval, match.length, lex_state);
            }
            else if (rule->tok)
            {
                token = rule->tok;
            }
            else
            {
                token = -1; // skip
            }

            *offset += match.length;
            if (token > 0)
                return token;

            i = 0;
        }
    }

    // Invalid token
    if (*offset < len)
    {
        (*offset)++;
        return -1;
    }

    return 0;
}