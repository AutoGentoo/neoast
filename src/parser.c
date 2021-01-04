//
// Created by tumbar on 12/25/20.
//

#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

U32 parser_init(GrammarParser* self)
{
    self->regex_opts = cre2_opt_new();
    cre2_opt_set_one_line(self->regex_opts, 1);

    U32 error = 0;
    for (U32 i = 0; i < self->lex_state_n; i++)
    {
        for (U32 j = 0; j < self->lex_n[i]; j++)
        {
            const char* pattern = self->lexer_rules[i][j].regex_raw;
            if ((self->lexer_rules[i][j].regex =
                    cre2_new(pattern, (U32)strlen(pattern), self->regex_opts)) == NULL) {
                fprintf(stderr, "Failed to compile regex \"%s\"\n", pattern);
                error++;
            }
        }
    }

    return error;
}

void parser_free(GrammarParser* self)
{
    for (U32 i = 0; i < self->lex_state_n; i++)
    {
        for (U32 j = 0; j < self->lex_n[i]; j++)
        {
            cre2_delete(self->lexer_rules[i][j].regex);
        }
    }

    cre2_opt_delete(self->regex_opts);
}

Stack* parser_allocate_stack(U64 stack_n)
{
    Stack* out = malloc(sizeof(Stack) + sizeof(U32) * stack_n);
    out->pos = 0;

    STACK_PUSH(out, 0);

    return out;
}

void parser_free_stack(Stack* self)
{
    free(self);
}