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


#include <neoast.h>
#include <alloca.h>
#include <string.h>
#include <assert.h>

#define OFFSET_VOID_PTR(ptr, s, i) (void*)(((char*)(ptr)) + ((s) * (i)))

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
        const uint32_t* parsing_table,
        uint32_t reduce_token,
        const ParserBuffers* buffers,
        uint32_t* dest_idx)
{
    // Find how many tokens to pop
    // due to this rule
    const GrammarRule* reduce_rule = &parser->grammar_rules[reduce_token & TOK_MASK];
    uint32_t arg_count = reduce_rule->tok_n;

    char* dest = alloca(buffers->val_s);
    char* args = alloca(buffers->val_s * arg_count);

    uint32_t idx = *dest_idx;
    assert(buffers->parsing_stack->pos % 2 == 1 && buffers->parsing_stack->pos > (arg_count << 1));
    for (uint32_t i = 0; i < arg_count; i++)
    {
        NEOAST_STACK_POP(buffers->parsing_stack); // Pop the state
        idx = NEOAST_STACK_POP(buffers->parsing_stack); // Pop the index of the token/value

        // Fill the argument
        memcpy(OFFSET_VOID_PTR(args, buffers->val_s, arg_count - i - 1),
               OFFSET_VOID_PTR(buffers->value_table, buffers->val_s, idx),
               buffers->val_s);
    }

    int32_t result_token = (int32_t)reduce_rule->token;
    result_token -= NEOAST_ASCII_MAX;
    assert(result_token > 0);

    // Run the reduction code
    parser->parser_reduce(reduce_token & TOK_MASK, dest, (void**) args);

    // Copy the result back into the table
    memcpy(OFFSET_VOID_PTR(buffers->value_table, buffers->val_s, idx),
           dest,
           buffers->val_s);

    assert(buffers->val_s - buffers->union_s >= sizeof(TokenPosition));
    if (arg_count > 0)
    {
        // Copy the positional data of the first argument back to the destination
        memcpy(dest + buffers->union_s, args + buffers->union_s, sizeof(TokenPosition));
    }
    else
    {
        // No argument (empty rule), no positional data available
        memset(dest + buffers->union_s, 0, sizeof(TokenPosition));
    }

    // Fill the result
    buffers->token_table[idx] = result_token;

    // Check the goto
    uint32_t next_state = *(uint32_t*) g_table_from_matrix(
            parsing_table,
            NEOAST_STACK_PEEK(buffers->parsing_stack), // Top of stack is current state
            result_token,
            parser->token_n);

    next_state &= TOK_MASK;

    NEOAST_STACK_PUSH(buffers->parsing_stack, idx);
    NEOAST_STACK_PUSH(buffers->parsing_stack, next_state);

    *dest_idx = idx;
    return next_state;
}

void lr_parse_error(const GrammarParser* self,
                    const uint32_t* parsing_table,
                    const TokenPosition* p,
                    uint32_t current_state,
                    uint32_t error_tok,
                    uint32_t prev_tok)
{
    const char* current_token = self->token_names[error_tok];
    const char* prev_token = self->token_names[prev_tok];

    uint32_t index_off = current_state * self->token_n;
    uint32_t* expected_tokens = alloca(sizeof(uint32_t) * self->token_n);
    uint32_t expected_tokens_n = 0;
    for (uint32_t i = 0; i < self->token_n; i++)
    {
        if (parsing_table[index_off + i] != TOK_SYNTAX_ERROR)
        {
            expected_tokens[expected_tokens_n++] = i;
        }
    }

    if (self->parser_error)
    {
        self->parser_error(
                self->token_names, p, prev_tok, error_tok,
                expected_tokens, expected_tokens_n);
    }
    else
    {
        if (p)
        {
            fprintf(stderr, "Error on line %d:%d\n", p->line, p->col_start);
        }

        fprintf(stderr, "Invalid syntax: unexpected token '%s' [%d] after '%s' [%d] (state %d)\n",
                current_token, error_tok, prev_token, prev_tok, current_state);
        fprintf(stderr, "Expected one of: ");

        for (uint32_t i = 0; i < expected_tokens_n; i++)
        {
            fprintf(stderr, "%s ", self->token_names[expected_tokens[i]]);
        }

        fprintf(stderr, "\n");
    }

}

static void run_destructor(const GrammarParser* parser,
                           const ParserBuffers* buffers,
                           uint32_t index)
{
    uint32_t current_token = buffers->token_table[index];

    if (current_token < parser->token_n         // Don't free invalid tokens
        && parser->destructors[current_token])  // Only free tokens that have a destructor
    {
        void* self_ptr = OFFSET_VOID_PTR(buffers->value_table,
                                         buffers->val_s, index);

        // A destructor is defined for this token
        // Call the destructor on this object
        parser->destructors[current_token](self_ptr);
        memset(self_ptr, 0, buffers->val_s);
    }
}

static void parser_run_destructors(
        const GrammarParser* parser,
        const ParserBuffers* buffers,
        int32_t invalid_tok_i)
{
    if (!parser->destructors)
    {
        // Destructors have not been defined
        return;
    }

    // Free the current invalid token
    if (invalid_tok_i >= 0)
    {
        run_destructor(parser, buffers, invalid_tok_i);
    }

    // Free the reduced tokens
    assert(buffers->parsing_stack->pos % 2 == 1);
    while (buffers->parsing_stack->pos > 1) // lastly, hold the initialized state '0'
    {
        NEOAST_STACK_POP(buffers->parsing_stack); // state
        uint32_t index = NEOAST_STACK_POP(buffers->parsing_stack);
        run_destructor(parser, buffers, index);
    }
}

int32_t parser_parse_lr(const GrammarParser* parser,
                        const uint32_t* parsing_table,
                        const ParserBuffers* buffers,
                        void* lexer,
                        int ll_next(void*, void*))
{
    // Lexer states
    char* lex_val = buffers->value_table;

    // Push the initial state to the stack
    uint32_t current_state = 0;
    NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);

    uint32_t i = 0;
    uint32_t prev_tok = 0;
    int32_t tok = ll_next(lexer, lex_val);
    buffers->token_table[0] = tok;

    uint32_t dest_idx = 0; // index of the last reduction
    while (1)
    {
        // Check for lexing error
        if (tok < 0)
        {
            parser_run_destructors(parser, buffers, -1);
            return -1;
        }

        uint32_t table_value = *(uint32_t*) g_table_from_matrix(
                parsing_table,
                current_state,
                tok,
                parser->token_n);

        if (table_value == TOK_SYNTAX_ERROR)
        {
            const TokenPosition* p = (const TokenPosition*)(lex_val + buffers->union_s);

            lr_parse_error(parser,
                           parsing_table,
                           p,
                           current_state,
                           tok, prev_tok);

            // We need to free the remaining objects in this map
            parser_run_destructors(parser, buffers, (int32_t)i);
            return -1;
        }
        else if (table_value & TOK_SHIFT_MASK)
        {
            current_state = table_value & TOK_MASK;
            NEOAST_STACK_PUSH(buffers->parsing_stack, i);
            NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);
            prev_tok = tok;

            lex_val += buffers->val_s;
            tok = ll_next(lexer, lex_val);
            buffers->token_table[++i] = tok;
        }
        else if (table_value & TOK_REDUCE_MASK)
        {
            // Goto rules cannot occur here
            assert(tok < parser->action_token_n);

            // Reduce this rule
            current_state = g_lr_reduce(parser, parsing_table,
                                        table_value, buffers,
                                        &dest_idx);

            // Don't move on empty rule
            if (i != dest_idx)
            {
                // Move the lookahead to the slot in front of the
                // result. We should do this so that we don't fill up the
                // buffer tables. As rules reduce we reduce our footprint
                // on the buffers.
                assert(buffers->token_table[i] == tok);
                void* dest_val = OFFSET_VOID_PTR(buffers->value_table, buffers->val_s, dest_idx + 1);
                memcpy(dest_val, lex_val, buffers->val_s);
                buffers->token_table[dest_idx + 1] = buffers->token_table[i];

                lex_val = dest_val;
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
