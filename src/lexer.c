/* 
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int32_t
lex_next(const char* input,
         const GrammarParser* parser,
         const ParserBuffers* buf,
         void* lval,
         uint32_t len,
         uint32_t* offset)
{
    while (1) // while tokens are being skipped
    {
        if (*offset >= len)
        {
            return 0;
        }

        uint32_t i = 0;
        int32_t longest_match = -1;
        uint32_t longest_match_length = 0;
        uint32_t state_index = NEOAST_STACK_PEEK(buf->lexing_state_stack);
        while(i < parser->lex_n[state_index])
        {
            cre2_string_t match;
            LexerRule* rule = &parser->lexer_rules[state_index][i];

            if (cre2_match(rule->regex, input, (int32_t)len,
                           (int32_t)*offset, (int32_t)len, CRE2_ANCHOR_START,
                           &match, 1))
            {
                if (match.length > longest_match_length)
                {
                    longest_match = (int32_t)i;
                    longest_match_length = match.length;
                }

                if (!(parser->lexer_opts & LEXER_OPT_LONGEST_MATCH))
                {
                    // Use the first match
                    break;
                }
            }

            i++;
        }

        if (longest_match >= 0)
        {
            int32_t token;
            LexerRule* rule = &parser->lexer_rules[state_index][longest_match];

            TokenPosition* position;
            if (parser->lexer_opts & LEXER_OPT_TOKEN_POS)
            {
                position = (TokenPosition*) (((char*)lval) + buf->union_s);
                position->line = buf->position->line;
                position->col_start = *offset - buf->position->line_start_offset + 1;
            }
            else
            {
                position = NULL;
            }

            if (rule->expr)
            {
                if (longest_match_length < buf->max_token_length)
                {
                    memcpy(buf->text_buffer, input + *offset, longest_match_length);
                    buf->text_buffer[longest_match_length] = 0;
                    token = rule->expr(buf->text_buffer, lval, longest_match_length, buf->lexing_state_stack, position);
                }
                else
                {
                    char* m_buff = strndup(input + *offset, longest_match_length);
                    token = rule->expr(m_buff, lval, longest_match_length, buf->lexing_state_stack, position);
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

            *offset += longest_match_length;
            if (parser->lexer_opts & LEXER_OPT_TOKEN_POS && buf->position->line != position->line)
            {
                buf->position->line = position->line;
                buf->position->line_start_offset = *offset;
            }

            if (token > 0)
            {
                // Check if this token is an ascii character and needs to be converted
                if (parser->ascii_mappings && token < NEOAST_ASCII_MAX)
                {
                    int32_t mapping = (int32_t)parser->ascii_mappings[token];
                    if (mapping <= NEOAST_ASCII_MAX)
                    {
                        fprintf(stderr, "Token '%c' needs to be explicitly defined\n", token);
                        exit(1);
                    }
                    return mapping - NEOAST_ASCII_MAX;
                } else if (!parser->ascii_mappings)
                {
                    // Ascii mappings are not defined
                    return token;
                } else
                {
                    return token - NEOAST_ASCII_MAX;
                }
            }
        }
        else
        {
            // No token was matched
            break;
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
