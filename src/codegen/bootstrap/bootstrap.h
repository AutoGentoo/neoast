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
 *
 * THIS FILE HAS BEEN AUTOMATICALLY GENERATED, DO NOT CHANGE
 */

#ifndef __NEOAST_CC_H__
#define __NEOAST_CC_H__


    #include <codegen/codegen.h>
    #include <codegen/compiler.h>
    #include <stdlib.h>
    #include <stddef.h>
    #include <string.h>
    #include <stdint.h>
    #include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

/*************************** NEOAST Token definition ****************************/
#ifdef NEOAST_GET_TOKENS
enum
{
    LL = 257,
    GG, // 258 0x102
    OPTION, // 259 0x103
    START, // 260 0x104
    UNION, // 261 0x105
    TYPE, // 262 0x106
    LEFT, // 263 0x107
    RIGHT, // 264 0x108
    TOP, // 265 0x109
    INCLUDE, // 266 0x10A
    END_STATE, // 267 0x10B
    MACRO, // 268 0x10C
    BOTTOM, // 269 0x10D
    TOKEN, // 270 0x10E
    DESTRUCTOR, // 271 0x10F
    ASCII, // 272 0x110
    LITERAL, // 273 0x111
    ACTION, // 274 0x112
    IDENTIFIER, // 275 0x113
    LEX_STATE, // 276 0x114
    ASCII_CHAR_0x3C, // 277 0x115 '<'
    ASCII_CHAR_0x3E, // 278 0x116 '>'
    ASCII_CHAR_0x3A, // 279 0x117 ':'
    ASCII_CHAR_0x7C, // 280 0x118 '|'
    ASCII_CHAR_0x3B, // 281 0x119 ';'
    ASCII_CHAR_0x3D, // 282 0x11A '='
    single_grammar, // 283 0x11B
    multi_grammar, // 284 0x11C
    grammar, // 285 0x11D
    grammars, // 286 0x11E
    header, // 287 0x11F
    pair, // 288 0x120
    lexer_rules, // 289 0x121
    lexer_rules_state, // 290 0x122
    lexer_rule, // 291 0x123
    tokens, // 292 0x124
    token, // 293 0x125
    program, // 294 0x126
    TOK_AUGMENT, // 295 0x127
};


#endif // NEOAST_GET_TOKENS

#include <lexer/input.h>

#ifdef NEOAST_GET_STRUCTURE
/************************ NEOAST Union/Struct definition ************************/
typedef union { 
    char* identifier;
    char ascii;
    key_val_t key_val_type;
    KeyVal* key_val;
    struct LexerRuleProto* l_rule;
    struct Token* token;
    struct GrammarRuleSingleProto* g_single_rule;
    struct GrammarRuleProto* g_rule;
    struct File* file;
    struct { char* string; TokenPosition position; } action;
 } NeoastUnion;

typedef struct {
    NeoastUnion value;
    TokenPosition position;
} NeoastValue;
#endif // NEOAST_GET_STRUCTURE

/*************************** NEOAST Lexer definition ****************************/


void* cc_allocate_buffers();

void cc_free_buffers(void* self);

extern NeoastUnion __cc__t_;

/**
 * Basic entrypoint into the cc parser
 * @param buffers_ Allocated pointer to buffers created with cc_allocate_buffers()
 * @param input pointer to raw input
 * @param input_len length of input in bytes
 * @return top of the generated AST
 */
typeof(__cc__t_.file) cc_parse_len(void* error_ctx, void* buffers_, const char* input, uint32_t input_len);
typeof(__cc__t_.file) cc_parse(void* error_ctx, void* buffers_, const char* input);

/**
 * Advanced entrypoint into the cc parser
 * @param buffers_ Allocated pointer to buffers created with cc_allocate_buffers()
 * @param input initialized neoast input from <lexer/input.h>
 * @return top of the generated AST
 */
typeof(__cc__t_.file) cc_parse_input(void* error_ctx, void* buffers_, NeoastInput* input);

#ifdef __cplusplus
}
#endif

#endif // __NEOAST_CC_H__
