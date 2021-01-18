//
// Created by tumbar on 12/25/20.
//

#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

uint32_t parser_init(GrammarParser* self)
{
    self->regex_opts = cre2_opt_new();
    cre2_opt_set_one_line(self->regex_opts, 1);

    uint32_t error = 0;
    for (uint32_t i = 0; i < self->lex_state_n; i++)
    {
        for (uint32_t j = 0; j < self->lex_n[i]; j++)
        {
            const char* pattern = self->lexer_rules[i][j].regex_raw;
            if ((self->lexer_rules[i][j].regex =
                    cre2_new(pattern, (uint32_t)strlen(pattern), self->regex_opts)) == NULL) {
                fprintf(stderr, "Failed to compile regex \"%s\"\n", pattern);
                error++;
            }
        }
    }

    return error;
}

void parser_free(GrammarParser* self)
{
    for (uint32_t i = 0; i < self->lex_state_n; i++)
    {
        for (uint32_t j = 0; j < self->lex_n[i]; j++)
        {
            cre2_delete(self->lexer_rules[i][j].regex);
        }
    }

    cre2_opt_delete(self->regex_opts);
}

ParsingStack* parser_allocate_stack(size_t stack_n)
{
    ParsingStack* out = malloc(sizeof(ParsingStack) + sizeof(uint32_t) * stack_n);
    out->pos = 0;

    return out;
}

void parser_free_stack(ParsingStack* self)
{
    free(self);
}

ParserBuffers* parser_allocate_buffers(int max_lex_tokens, int max_token_len, int max_lex_state_depth, size_t val_s)
{
    ParserBuffers* buffers = malloc(sizeof(ParserBuffers));
    buffers->lexing_state_stack = parser_allocate_stack(max_lex_state_depth);
    buffers->parsing_stack = parser_allocate_stack(256);

    buffers->token_table = malloc(sizeof(uint32_t) * max_lex_tokens);
    buffers->value_table = malloc(val_s * max_lex_tokens);
    buffers->text_buffer = malloc(max_token_len);
    buffers->max_token_length = max_token_len;
    buffers->val_s = val_s;
    buffers->table_n = max_lex_tokens;

    return buffers;
}

void parser_free_buffers(ParserBuffers* self)
{
    parser_free_stack(self->parsing_stack);
    parser_free_stack(self->lexing_state_stack);
    free(self->token_table);
    free(self->value_table);
    free(self->text_buffer);
    free(self);
}

void parser_reset_buffers(const ParserBuffers* self)
{
    self->parsing_stack->pos = 0;
    self->lexing_state_stack->pos = 0;
}