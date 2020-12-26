//
// Created by tumbar on 12/23/20.
//

#include <parser.h>
#include <alloca.h>
#include <string.h>

static inline
void* g_table_from_matrix(void* table,
                          size_t row, size_t col,
                          size_t col_n, size_t s)
{
    size_t off = s * ((row * col_n) + (col));
    return ((char*) table) + off;
}

static inline
U32 g_lr_reduce(
        const GrammarParser* parser,
        Stack* stack,
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
        STACK_POP(stack); // Pop the state
        idx = STACK_POP(stack); // Pop the index of the token/value

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
            STACK_PEEK(stack), // Top of stack is current state
            result_token,
            parser->token_n,
            sizeof(U32));

    next_state &= TOK_MASK;

    STACK_PUSH(stack, idx);
    STACK_PUSH(stack, next_state);

    *dest_idx = idx;
    return next_state;
}

void lr_parse_error(const U32* parsing_table,
                    const char* token_names[],
                    U32 current_state,
                    U32 error_tok,
                    U32 prev_tok,
                    U32 tok_n)
{
    const char* current_token = token_names[error_tok];
    const char* prev_token = token_names[prev_tok];

    fprintf(stderr, "Invalid syntax: unexpected token '%s' after '%s' (state %d)\n",
            current_token, prev_token, current_state);
    fprintf(stderr, "Expected one of: ");

    U32 index_off = current_state * tok_n;
    for (U32 i = 0; i < tok_n; i++)
    {
        if (parsing_table[index_off + i] != TOK_SYNTAX_ERROR)
        {
            fprintf(stderr, "%s,", token_names[i]);
        }
    }

    fprintf(stderr, "\n");
}

I32 parser_parse_lr(
        const GrammarParser* parser,
        Stack* stack,
        const U32* parsing_table,
        const char* token_names[],
        U32* token_table,
        void* val_table,
        size_t val_s)
{
    // Push the initial state to the stack
    U32 current_state = 0;
    STACK_PUSH(stack, current_state);

    U32 i = 0;
    U32 prev_tok = 0;
    U32 tok = token_table[i];
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
            lr_parse_error(parsing_table,
                           token_names, current_state,
                           tok, prev_tok,
                           parser->token_n);
            return -1;
        } else if (table_value & TOK_SHIFT_MASK)
        {
            printf("S%02d '%s'\n", table_value & TOK_MASK, token_names[tok]);
            current_state = table_value & TOK_MASK;
            STACK_PUSH(stack, i);
            STACK_PUSH(stack, current_state);
            prev_tok = tok;
            tok = token_table[++i];
        } else if (table_value & TOK_REDUCE_MASK)
        {
            // Reduce this rule
            current_state = g_lr_reduce(parser, stack, parsing_table,
                                        table_value & TOK_MASK,
                                        token_table, val_table, val_s,
                                        &dest_idx);
            prev_tok = parser->grammar_rules[table_value & TOK_MASK].token;
            printf("R%02d -> %s\n", table_value & TOK_MASK, token_names[prev_tok]);
            printf("G%02d\n", current_state);
        } else if (table_value & TOK_ACCEPT_MASK)
        {
            return dest_idx;
        }
    }
}