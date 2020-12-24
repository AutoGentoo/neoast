//
// Created by tumbar on 12/22/20.
//

#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

static inline
int tok_match(const U32* haystack,
              const U32* needle)
{
    int m = 0;
    while (*haystack && *needle)
    {
        if (*haystack++ != *needle++)
            return -1;
        m++;
    }

    if (*needle)
        return -1; // EOF
    return m;
}

static
int parser_left_scan(
        const GrammarParser* parser,
        int* n,
        U32* tokens,
        void* val_table,
        U32 val_s)
{
    // Create a buffer to hold the output union
    void* dest = alloca(val_s);

    int match_count = 0;
    for (int i = 0; i < *n; i++)
    {
        // Try to match each rule
        for (int j = 0; j < parser->grammar_n; j++)
        {
            int tok_n = tok_match(&tokens[i], parser->grammar_rules[j].grammar);
            if (tok_n >= 0)
            {
                match_count++;

                // We matched the rule
                // Reduce the rule
                int reduced_token = parser->grammar_rules[j].expr(
                        dest,
                        OFFSET_VOID_PTR(val_table, val_s, i));

                // TODO Run destructors for each in val_table[tok_match:tok_match+tok_n]

                // Update the values in the tables
                tokens[i] = reduced_token;
                memcpy(OFFSET_VOID_PTR(val_table, val_s, i), dest, val_s);

                // Compress the tables
                if (tok_n > 1 && i + tok_n < *n)
                {
                    // We need to copy the tokens after this
                    // because there are tokens/values after this
                    int src_idx = i + tok_n;
                    int dest_idx = i + 1;
                    int idx_n = *n - i - tok_n + 1;

                    memmove(&tokens[dest_idx], &tokens[src_idx], sizeof(int) * (idx_n));
                    memmove(OFFSET_VOID_PTR(val_table, val_s, dest_idx),
                            OFFSET_VOID_PTR(val_table, val_s, src_idx),
                            val_s * idx_n);
                } else if (i + tok_n == *n)
                {
                    // This expression reaches the last token so we can just
                    // terminate the tokens with null
                    tokens[i + 1] = 0;
                } else
                {
                    // This is a single token expression
                    // We don't need to copy anything
                }

                // We consume all the tokens in the expression
                // but then we create a return value token
                *n -= tok_n - 1;

                // Don't match anymore rules here
                break;
            }
        }
    }

    return match_count;
}


void parser_parse_left_scan(
        const GrammarParser* parser,
        int* n,
        U32* tokens,
        void* val_table,
        U32 val_s)
{
    while (parser_left_scan(parser, n, tokens, val_table, val_s));
}
