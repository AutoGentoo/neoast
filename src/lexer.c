//
// Created by tumbar on 12/22/20.
//

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"

int32_t lexer_fill_table(const char* input, size_t len, const GrammarParser* parse, const ParserBuffers* buf)
{
    void* current_val = buf->value_table;
    int i = 0;
    STACK_PUSH(buf->lexing_state_stack, 0);
    uint32_t offset = 0;
    while ((buf->token_table[i] = lex_next(input, parse, buf, current_val, len, &offset)) && i < buf->table_n) // While not EOF
    {
        if (buf->token_table[i] == -1)
        {
            fprintf(stderr, "Unmatched token near '%.*s' (state '%d')\n",
                    (uint32_t)(strchr(&input[offset - 1], '\n') - &input[offset - 1]),
                    &input[offset - 1], STACK_PEEK(buf->lexing_state_stack));
            return -1;
        }

        current_val += buf->val_s;
        i++;
    }

    return i;
}

int lex_next(const char* input, const GrammarParser* parser, const ParserBuffers* buf, void* lval, uint32_t len, uint32_t* offset)
{
    if (*offset >= len)
        return 0;

    cre2_string_t match;
    LexerRule* rule;

    int i = 0;
    uint32_t state_index = STACK_PEEK(buf->lexing_state_stack);
    while(i < parser->lex_n[state_index])
    {
        rule = &parser->lexer_rules[state_index][i++];

        if (cre2_match(rule->regex, input, len,
                        *offset, len, CRE2_ANCHOR_START,
                        &match, 1))
        {
            int32_t token;

            if (rule->expr)
            {
                if (match.length < buf->max_token_length)
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
            {
                // Check if this token is an ascii character and needs to be converted
                if (parser->ascii_mappings && token < ASCII_MAX)
                {
                    int32_t mapping = parser->ascii_mappings[token];
                    if (mapping <= ASCII_MAX)
                    {
                        fprintf(stderr, "Token '%c' needs to be explicitly defined\n", token);
                        exit(1);
                    }
                    return mapping - ASCII_MAX;
                }
                else if (!parser->ascii_mappings)
                {
                    // Ascii mappings are not defined
                    return token;
                }
                else
                {
                    return token - ASCII_MAX;
                }
            }

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