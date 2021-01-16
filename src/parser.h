//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_PARSER_H
#define NEOAST_PARSER_H
#include <stdio.h>
#include "common.h"
#include <neoast.h>

#define OFFSET_VOID_PTR(ptr, s, i) (void*)(((char*)(ptr)) + ((s) * (i)))

/**
 * Run the LR parsing algorithm
 * given a parser with the parsing
 * table filled.
 * @param parser target parser (kept constant)
 * @param stack used for parsing
 * @param token_table tokens from lexer
 * @param val_table values from lexer
 * @param val_s sizeof each value
 * @return index in token/value table where the parsed value resides
 */
int32_t parser_parse_lr(
        const GrammarParser* parser,
        const uint32_t* parsing_table,
        const ParserBuffers* buffers);

uint32_t parser_init(GrammarParser* self);
void parser_free(GrammarParser* self);

ParsingStack* parser_allocate_stack(uint64_t stack_n);
void parser_free_stack(ParsingStack* self);
ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len, int max_lex_state_depth, size_t val_s);
void parser_free_buffers(ParserBuffers* self);
void parser_reset_buffers(const ParserBuffers* self);

#endif //NEOAST_PARSER_H
