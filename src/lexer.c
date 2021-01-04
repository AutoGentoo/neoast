//
// Created by tumbar on 12/22/20.
//

#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

U32 lexer_fill_table(const char* input, U64 len, const GrammarParser* parse, U32* table, void* val_table, U32 val_n,
                     U32 table_n)
{
    void* current_val = val_table;
    int i = 0;
    Stack* lex_state = parser_allocate_stack(32);
    STACK_PUSH(lex_state, 0);
    U32 offset = 0;
    while ((table[i] = lex_next(input, parse, current_val, len, &offset, lex_state)) && i < table_n) // While not EOF
    {
        if (table[i] == -1)
        {
            fprintf(stderr, "Invalid character '%c' (state '%d')\n",
                    input[offset - 1], STACK_PEEK(lex_state));
            continue;
        }

        current_val += val_n;
        i++;
    }
    parser_free_stack(lex_state);

    return i;
}

int lex_next(const char* input,
             const GrammarParser* parser,
             void* lval,
             U32 len,
             U32* offset,
             Stack* lex_state)
{
    if (*offset >= len)
        return 0;

    cre2_string_t match;
    LexerRule* rule;

    static __thread char lex_buffer[1024];

    int i = 0;
    U32 state_index = STACK_PEEK(lex_state);
    while(i < parser->lex_n[state_index])
    {
        rule = &parser->lexer_rules[state_index][i++];

        if (cre2_match(rule->regex, input, len,
                        *offset, len, CRE2_ANCHOR_START,
                        &match, 1))
        {
            I32 token;

            if (rule->expr)
            {
                if (match.length < 1024)
                {
                    memcpy(lex_buffer, input + *offset, match.length);
                    lex_buffer[match.length] = 0;
                    token = rule->expr(match.data, lval, match.length, lex_state);
                }
                else
                {
                    char* m_buff = strndup(match.data, match.length);
                    token = rule->expr(m_buff, lval, match.length, lex_state);
                    free(m_buff);
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

            *offset += match.length;
            if (token > 0)
                return token;

            state_index = STACK_PEEK(lex_state);
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