//
// Created by tumbar on 12/22/20.
//

#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

U32 lexer_fill_table(const char* input, U64 len, const GrammarParser* parse, const ParserBuffers* buf)
{
    void* current_val = buf->value_table;
    int i = 0;
    STACK_PUSH(buf->lexing_state_stack, 0);
    U32 offset = 0;
    while ((buf->token_table[i] = lex_next(input, parse, buf, current_val, len, &offset)) && i < buf->table_n) // While not EOF
    {
        if (buf->token_table[i] == -1)
        {
            fprintf(stderr, "Invalid character '%c' (state '%d')\n",
                    input[offset - 1], STACK_PEEK(buf->lexing_state_stack));
            continue;
        }

        current_val += buf->val_s;
        i++;
    }

    return i;
}

int lex_next(const char* input, const GrammarParser* parser, const ParserBuffers* buf, void* lval, U32 len, U32* offset)
{
    if (*offset >= len)
        return 0;

    cre2_string_t match;
    LexerRule* rule;

    int i = 0;
    U32 state_index = STACK_PEEK(buf->lexing_state_stack);
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
                    memcpy(buf->text_buffer, input + *offset, match.length);
                    buf->text_buffer[match.length] = 0;
                    token = rule->expr(buf->text_buffer, lval, match.length, buf->lexing_state_stack);
                }
                else
                {
                    char* m_buff = strndup(match.data, match.length);
                    token = rule->expr(m_buff, lval, match.length, buf->lexing_state_stack);
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

            state_index = STACK_PEEK(buf->lexing_state_stack);
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