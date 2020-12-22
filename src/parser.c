//
// Created by tumbar on 12/22/20.
//

#include "parser.h"
#include "lexer.h"

__thread static int token_table[NEOAST_TOKEN_T_N];

void parser_fill_table(const char** input,
                       GrammarParser* parse,
                       int * table,
                       void* val_table,
                       size_t val_n)
{
    void* current_val = val_table;
    int i = 0;
    while ((table[i] = lex_next(input, parse, current_val)) && i < NEOAST_TOKEN_T_N) // While not EOF
    {
        current_val += val_n;
        i++;
    }
}
