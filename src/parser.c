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
#include <neoast.h>

uint32_t parser_init(GrammarParser* self)
{
    (void) self;

    // TODO Possible initialization?
    return 0;
}

void parser_free(GrammarParser* self)
{
    (void) self;

    if (self->lexer_free && self->lexer_ptr)
    {
        self->lexer_free(self->lexer_ptr);
    }
}

ParsingStack* parser_allocate_stack(size_t stack_n)
{
    ParsingStack* out = malloc(sizeof(ParsingStack) + sizeof(uint32_t) * stack_n);
    out->pos = 0;

    return out;
}

void parser_free_stack(ParsingStack* self)
{
    free(self);
}

ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len,
                                       int parsing_stack_n,
                                       size_t val_s,
                                       size_t union_s)
{
    ParserBuffers* buffers = malloc(sizeof(ParserBuffers));
    buffers->parsing_stack = parser_allocate_stack(parsing_stack_n);
    buffers->token_table = malloc(sizeof(uint32_t) * max_lex_tokens);
    buffers->value_table = malloc(val_s * max_lex_tokens);
    buffers->max_token_length = max_token_len;
    buffers->val_s = val_s;
    buffers->union_s = union_s;
    buffers->table_n = max_lex_tokens;

    return buffers;
}

void parser_free_buffers(ParserBuffers* self)
{
    parser_free_stack(self->parsing_stack);
    free(self->token_table);
    free(self->value_table);
    free(self);
}

void parser_reset_buffers(const ParserBuffers* self)
{
    self->parsing_stack->pos = 0;
}
