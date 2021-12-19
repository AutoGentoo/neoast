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


#ifndef NEOAST_BOOTSTRAP_LEXER_H
#define NEOAST_BOOTSTRAP_LEXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <neoast.h>
#include <stdint.h>

typedef int (*lexer_expr) (
        const char* lex_text,
        void* lex_val,
        uint32_t len,
        ParsingStack* ll_state,
        TokenPosition* position);


typedef struct
{
    const char* regex;
    lexer_expr expr;
    int tok;
} LexerRule;

void* bootstrap_lexer_new(const LexerRule* rules[], const uint32_t rules_n[],
                          uint32_t state_n, ll_eor_cb error_cb,
                          size_t position_offset, const uint32_t* ascii_mappings);
void bootstrap_lexer_free(void* lexer);

void* bootstrap_lexer_instance_new(const void* lexer_parent, void* err_ctx, const char* input, size_t length);

int bootstrap_lexer_next(void* lexer, void* ll_val);
void bootstrap_lexer_instance_free(void* lexer_inst);

#ifdef __cplusplus
};
#endif

#endif //NEOAST_BOOTSTRAP_LEXER_H
