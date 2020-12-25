//
// Created by tumbar on 12/25/20.
//

#ifndef NEOAST_CODEGEN_H
#define NEOAST_CODEGEN_H

typedef union {
    struct Option
    {
        char* option_name;
        char* value;
        struct Option* next;
    } option;

    char* string;
} LexerUnion;

void gen_parser_init(GrammarParser* self);

extern U32* GEN_lexer_table;

#endif //NEOAST_CODEGEN_H
