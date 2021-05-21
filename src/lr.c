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

static inline
void* g_table_from_matrix(void* table,
                          size_t row, size_t col,
                          size_t col_n, size_t s)
{
    size_t off = s * ((row * col_n) + (col));
    return ((char*) table) + off;
}

static inline
uint32_t g_lr_reduce(
        const GrammarParser* parser,
        ParsingStack* stack,
        const uint32_t* parsing_table,
        uint32_t reduce_rule,
        uint32_t* token_table,
        void* val_table,
        size_t val_s,
        uint32_t* dest_idx
)
{
    // Find how many tokens to pop
    // due to this rule
    uint32_t arg_count = parser->grammar_rules[reduce_rule].tok_n;

    void* dest = alloca(val_s);
    void* args = alloca(val_s * arg_count);

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

    int result_token = (int)parser->grammar_rules[reduce_rule].token;
    if (parser->ascii_mappings)
    {
        result_token -= NEOAST_ASCII_MAX;
        assert(result_token > 0);
    }
    if (parser->grammar_rules[reduce_rule].expr)
    {
        parser->grammar_rules[reduce_rule].expr(
                dest,
                args);
        memcpy(OFFSET_VOID_PTR(val_table, val_s, idx),
               dest,
               val_s);
    }

    // Fill the result
    token_table[idx] = result_token;

    // Check the goto
    uint32_t next_state = *(uint32_t*) g_table_from_matrix(
            (void*) parsing_table,
            NEOAST_STACK_PEEK(stack), // Top of stack is current state
            result_token,
            parser->token_n,
            sizeof(uint32_t));

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

static void parser_run_destructors(
        const GrammarParser* parser,
        const ParserBuffers* buffers,
        uint32_t i)
{
    // Tokens ahead of i are 'packed' and all of them need to be freed
    // Tokens behind i need to be freed based on the contents of the parsing stack
    // Tokens behind i are reduced tokens

    if (!parser->destructors)
    {
        // Destructors have not been defined
        return;
    }

    // First free the tokens ahead of i
    uint32_t current_token;
    while ((current_token = buffers->token_table[i])) // search for EOF
    {
        assert(current_token < parser->token_n);
        if (parser->destructors[current_token])
        {
            // A destructor is defined for this token
            // Call the destructor on this object
            parser->destructors[current_token](
                    OFFSET_VOID_PTR(buffers->value_table,
                                    buffers->val_s,
                                    i));
        }

        i++;
    }

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

int32_t parser_parse_lr(
        const GrammarParser* parser,
        const uint32_t* parsing_table,
        const ParserBuffers* buffers)
{
    // Push the initial state to the stack
    uint32_t current_state = 0;
    NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);

    uint32_t i = 0;
    uint32_t prev_tok = 0;
    uint32_t tok = buffers->token_table[i];
    uint32_t dest_idx = 0; // index of the last reduction
    while (1)
    {
        uint32_t table_value = *(uint32_t*) g_table_from_matrix(
                (void*)parsing_table,
                current_state,
                tok,
                parser->token_n,
                sizeof(uint32_t));

        if (table_value == TOK_SYNTAX_ERROR)
        {
            lr_parse_error(parsing_table,
                           parser->token_names, current_state,
                           tok, prev_tok,
                           parser->token_n);

            // We need to free the remaining objects in this map
            parser_run_destructors(parser, buffers, i);
            return -1;
        }
        else if (table_value & TOK_SHIFT_MASK)
        {
            current_state = table_value & TOK_MASK;
            NEOAST_STACK_PUSH(buffers->parsing_stack, i);
            NEOAST_STACK_PUSH(buffers->parsing_stack, current_state);
            prev_tok = tok;
            tok = buffers->token_table[++i];
        }
        else if (table_value & TOK_REDUCE_MASK)
        {
            // Reduce this rule
            current_state = g_lr_reduce(parser, buffers->parsing_stack, parsing_table,
                                        table_value & TOK_MASK,
                                        buffers->token_table, buffers->value_table, buffers->val_s,
                                        &dest_idx);
            prev_tok = parser->grammar_rules[table_value & TOK_MASK].token - NEOAST_ASCII_MAX;
        }
        else if (table_value & TOK_ACCEPT_MASK)
        {
            return (int32_t)dest_idx;
        }
    }
}
