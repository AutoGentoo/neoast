//
// Created by tumbar on 12/23/20.
//

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
        STACK_POP(stack); // Pop the state
        idx = STACK_POP(stack); // Pop the index of the token/value

        // Fill the argument
        memcpy(OFFSET_VOID_PTR(args, val_s, arg_count - i - 1),
               OFFSET_VOID_PTR(val_table, val_s, idx),
               val_s);
    }

    int result_token = (int)parser->grammar_rules[reduce_rule].token;
    if (parser->ascii_mappings)
    {
        result_token -= ASCII_MAX;
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
            STACK_PEEK(stack), // Top of stack is current state
            result_token,
            parser->token_n,
            sizeof(uint32_t));

    next_state &= TOK_MASK;

    STACK_PUSH(stack, idx);
    STACK_PUSH(stack, next_state);

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

    fprintf(stderr, "Invalid syntax: unexpected token '%s' after '%s' (state %d)\n",
            current_token, prev_token, current_state);
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

int32_t parser_parse_lr(
        const GrammarParser* parser,
        const uint32_t* parsing_table,
        const ParserBuffers* buffers)
{
    // Push the initial state to the stack
    uint32_t current_state = 0;
    STACK_PUSH(buffers->parsing_stack, current_state);

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
            return -1;
        } else if (table_value & TOK_SHIFT_MASK)
        {
            current_state = table_value & TOK_MASK;
            STACK_PUSH(buffers->parsing_stack, i);
            STACK_PUSH(buffers->parsing_stack, current_state);
            prev_tok = tok;
            tok = buffers->token_table[++i];
        } else if (table_value & TOK_REDUCE_MASK)
        {
            // Reduce this rule
            current_state = g_lr_reduce(parser, buffers->parsing_stack, parsing_table,
                                        table_value & TOK_MASK,
                                        buffers->token_table, buffers->value_table, buffers->val_s,
                                        &dest_idx);
            prev_tok = parser->grammar_rules[table_value & TOK_MASK].token;
        } else if (table_value & TOK_ACCEPT_MASK)
        {
            return dest_idx;
        }
    }
}