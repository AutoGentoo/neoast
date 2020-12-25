//
// Created by tumbar on 12/23/20.
//

#include <parser.h>
#include <alloca.h>
#include <string.h>

#ifndef NDEBUG
#include <assert.h>
#endif

static inline
void gs_push(ParserStack* stack, U32 item)
{
    stack->data[stack->pos++] = item;
}

static inline
U32 gs_pop(ParserStack* stack)
{
#ifndef NDEBUG
    assert(stack->pos);
#endif
    return stack->data[--stack->pos];
}

static inline
void* g_table_from_matrix(void* table,
                          size_t row, size_t col,
                          size_t col_n, size_t s
)
{
    size_t off = s * ((row * col_n) + (col));
    return ((char*) table) + off;
}

static inline
U32 g_lr_reduce(
        const GrammarParser* parser,
        ParserStack* stack,
        const U32* parsing_table,
        U32 reduce_rule,
        U32* token_table,
        void* val_table,
        size_t val_s,
        U32* dest_idx
)
{
    // Find how many tokens to pop
    // due to this rule
    U32 arg_count = parser->grammar_rules[reduce_rule].tok_n;

    void* dest = alloca(val_s);
    void* args = alloca(val_s * arg_count);

    U32 idx;
    for (U32 i = 0; i < arg_count; i++)
    {
        (void) gs_pop(stack); // Pop the state
        idx = gs_pop(stack); // Pop the index of the token/value

        // Fill the argument
        memcpy(OFFSET_VOID_PTR(args, val_s, i),
               OFFSET_VOID_PTR(val_table, val_s, idx),
               val_s);
    }

    if (parser->grammar_rules[reduce_rule].expr)
    {
        parser->grammar_rules[reduce_rule].expr(
                dest,
                args);
    }

    int result_token = parser->grammar_rules[reduce_rule].token;

    // Fill the result
    token_table[idx] = result_token;
    memcpy(OFFSET_VOID_PTR(val_table, val_s, idx),
           dest,
           val_s);

    U32 next_state = *(U32*) g_table_from_matrix(
            (void*) parsing_table,
            stack->data[stack->pos - 1],
            result_token,
            parser->token_n,
            sizeof(U32)) & TOK_MASK;

    gs_push(stack, idx);
    gs_push(stack, next_state);

    *dest_idx = idx;
    return next_state;
}

I32 parser_parse_lr(
        const GrammarParser* parser,
        ParserStack* stack,
        const U32* parsing_table,
        U32* token_table,
        void* val_table,
        size_t val_s)
{
    // Push the initial state to the stack
    U32 current_state = 0;
    gs_push(stack, current_state);

    U32 i = 0;
    U32 tok = token_table[i++];
    U32 dest_idx;
    while (1)
    {
        U32 table_value = *(U32*) g_table_from_matrix(
                (void*)parsing_table,
                current_state,
                tok,
                parser->token_n,
                sizeof(U32));

        if (table_value == TOK_SYNTAX_ERROR)
        {
            // TODO Generate better errors
            fprintf(stderr, "Unexpected token '%d' in state '%d'!\n", tok, current_state);
            return -1;
        } else if (table_value & TOK_SHIFT_MASK)
        {
            current_state = table_value & TOK_MASK;
            gs_push(stack, i - 1);
            gs_push(stack, current_state);
            tok = token_table[i++];
        } else if (table_value & TOK_REDUCE_MASK)
        {
            // Reduce this rule
            current_state = g_lr_reduce(parser, stack, parsing_table,
                                        table_value & TOK_MASK,
                                        token_table, val_table, val_s,
                                        &dest_idx);
        } else if (table_value & TOK_ACCEPT_MASK)
        {
            return dest_idx;
        }
    }
}