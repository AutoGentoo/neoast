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


#include <parser.h>
#include <alloca.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"

static inline
const void* g_table_from_matrix(const void* table,
                                size_t row, size_t col,
                                size_t col_n)
{
    size_t off = sizeof(uint32_t) * ((row * col_n) + (col));
    return ((const char*) table) + off;
}

static inline
uint32_t g_lr_reduce(
        const GrammarParser* parser,
        ParsingStack* stack,
        const uint32_t* parsing_table,
        const GrammarRule* reduce_rule,
        int32_t* token_table,
        void* val_table,
        size_t val_s,
        size_t union_s,
        uint32_t* dest_idx)
{
    // Find how many tokens to pop
    // due to this rule
    uint32_t arg_count = reduce_rule->tok_n;

    char* dest = alloca(val_s);
    char* args = alloca(val_s * arg_count);

    uint32_t idx = *dest_idx;
    for (uint32_t i = 0; i < arg_count; i++)
    {
        NEOAST_STACK_POP(stack); // Pop the state
        idx = NEOAST_STACK_POP(stack); // Pop the index of the token/value

        // Fill the argument
        memcpy(OFFSET_VOID_PTR(args, val_s, arg_count - i - 1),
               OFFSET_VOID_PTR(val_table, val_s, idx),
               val_s);
    }

    int32_t result_token = (int32_t)reduce_rule->token;
    if (parser->ascii_mappings)
    {
        result_token -= NEOAST_ASCII_MAX;
        assert(result_token > 0);
    }

    if (reduce_rule->expr)
    {
        reduce_rule->expr(
                dest,
                (void**) args);

        // Copy the result back into the table
        memcpy(OFFSET_VOID_PTR(val_table, val_s, idx),
               dest,
               val_s);

        if (parser->lexer_opts & LEXER_OPT_TOKEN_POS)
        {
            assert(val_s - union_s >= sizeof(TokenPosition));
            if (arg_count > 0)
            {
                // Copy the positional data of the first argument back to the destination
                memcpy(dest + union_s, args + union_s, sizeof(TokenPosition));
            }
            else
            {
                // No argument (empty rule), no positional data available
                memset(dest + union_s, 0, sizeof(TokenPosition));
            }
        }
    }

    // Fill the result
    token_table[idx] = result_token;

    // Check the goto
    uint32_t next_state = *(uint32_t*) g_table_from_matrix(
            parsing_table,
            NEOAST_STACK_PEEK(stack), // Top of stack is current state
            result_token,
            parser->token_n);

    next_state &= TOK_MASK;

    NEOAST_STACK_PUSH(stack, idx);
    NEOAST_STACK_PUSH(stack, next_state);

    *dest_idx = idx;
    return next_state;
}

void lr_parse_error(const uint32_t* parsing_table,
                    const char* const* token_names,
                    uint32_t current_state,
                    uint32_t error_tok,
                    uint32_t prev_tok,
                    uint32_t tok_n)
{
    const char* current_token = token_names[error_tok];
    const char* prev_token = token_names[prev_tok];

    fprintf(stderr, "Invalid syntax: unexpected token '%s' [%d] after '%s' [%d] (state %d)\n",
            current_token, error_tok, prev_token, prev_tok, current_state);
    fprintf(stderr, "Expected one of: ");

    uint32_t index_off = current_state * tok_n;
    for (uint32_t i = 0; i < tok_n; i++)
    {
        if (parsing_table[index_off + i] != TOK_SYNTAX_ERROR)
        {
            fprintf(stderr, "%s ", token_names[i]);
        }
    }

    fprintf(stderr, "\n");
}

void lr_lex_error(const ParserBuffers* buf,
                  const char* input,
                  uint32_t offset)
{
    fprintf(stderr, "Unmatched token near '%.*s' (state '%d')\n",
            (uint32_t)(strchr(&input[offset - 1], '\n') - &input[offset - 1]),
            &input[offset - 1],
            NEOAST_STACK_PEEK(buf->lexing_state_stack));
}

static void parser_run_destructors(
        const GrammarParser* parser,
        const ParserBuffers* buffers)
{
    if (!parser->destructors)
    {
        // Destructors have not been defined
        return;
    }

    uint32_t current_token;

    // Free the reduced tokens
    assert(buffers->parsing_stack->pos % 2 == 1);
    while (buffers->parsing_stack->pos > 1) // last hold the initialize state '0'
    {
        NEOAST_STACK_POP(buffers->parsing_stack); // state
        uint32_t index = NEOAST_STACK_POP(buffers->parsing_stack);
        current_token = buffers->token_table[index];

        assert(current_token < parser->token_n);
        if (parser->destructors[current_token])
        {
            void* self_ptr = OFFSET_VOID_PTR(buffers->value_table,
                                             buffers->val_s, index);

            // A destructor is defined for this token
            // Call the destructor on this object
            parser->destructors[current_token](self_ptr);
            memset(self_ptr, 0, buffers->val_s);
        }
    }
}

int32_t parser_parse_lr(const GrammarParser* parser,
                        const uint32_t* parsing_table,
                        const ParserBuffers* buffers,
                        const char* input,
                        size_t length)
{
    // Lexer states
    uint32_t offset = 0;
    void* lex_val = buffers->value_table;
    NEOAST_STACK_PUSH(buffers->lexing_state_stack, 0);

    // Push the initial state to the stack
    uint32_t current_state = 0;
    NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);

    uint32_t i = 0;
    uint32_t prev_tok = 0;
    int32_t tok = lex_next(input, parser, buffers, lex_val, length, &offset);
    buffers->token_table[0] = tok;

    uint32_t dest_idx = 0; // index of the last reduction
    while (1)
    {
        // Check for lexing error
        if (tok < 0)
        {
            // TODO Enable user specified function
            lr_lex_error(buffers, input, offset);
            parser_run_destructors(parser, buffers);
            return -1;
        }

        uint32_t table_value = *(uint32_t*) g_table_from_matrix(
                parsing_table,
                current_state,
                tok,
                parser->token_n);

        if (table_value == TOK_SYNTAX_ERROR)
        {
            // TODO Enable user specified function
            lr_parse_error(parsing_table,
                           parser->token_names, current_state,
                           tok, prev_tok,
                           parser->token_n);

            // We need to free the remaining objects in this map
            parser_run_destructors(parser, buffers);
            return -1;
        }
        else if (table_value & TOK_SHIFT_MASK)
        {
            current_state = table_value & TOK_MASK;
            NEOAST_STACK_PUSH(buffers->parsing_stack, i);
            NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);
            prev_tok = tok;

            lex_val += buffers->val_s;
            tok = lex_next(input, parser, buffers, lex_val, length, &offset);
            buffers->token_table[++i] = tok;
        }
        else if (table_value & TOK_REDUCE_MASK)
        {
            // Reduce this rule
            const GrammarRule* reduce_rule = &parser->grammar_rules[table_value & TOK_MASK];
            current_state = g_lr_reduce(parser, buffers->parsing_stack, parsing_table,
                                        reduce_rule,
                                        buffers->token_table, buffers->value_table,
                                        buffers->val_s, buffers->union_s,
                                        &dest_idx);

            // Don't move on empty rule
            if (i != dest_idx)
            {
                // Move the lookahead to the slot in front of the
                // result. We should do this so that we don't fill up the
                // buffer tables. As rules reduce we reduce our footprint
                // on the buffers.
                assert(buffers->token_table[i] == tok);
                lex_val = OFFSET_VOID_PTR(buffers->value_table, buffers->val_s, dest_idx + 1);
                memcpy(lex_val,
                       OFFSET_VOID_PTR(buffers->value_table, buffers->val_s, i),
                       buffers->val_s);

                buffers->token_table[dest_idx + 1] = buffers->token_table[i];

                i = dest_idx + 1;
            }

            prev_tok = parser->grammar_rules[table_value & TOK_MASK].token - NEOAST_ASCII_MAX;
        }
        else if (table_value & TOK_ACCEPT_MASK)
        {
            return (int32_t)dest_idx;
        }
    }
}
