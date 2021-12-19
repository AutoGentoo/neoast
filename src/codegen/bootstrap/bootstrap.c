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

#include <neoast.h>
#include <string.h>

#define NEOAST_GET_TOKENS
#define NEOAST_GET_STRUCTURE
#ifndef __NEOAST_CC_H__
#define __NEOAST_CC_H__


    #include <codegen/codegen.h>
    #include <codegen/compiler.h>
    #include <stdlib.h>
    #include <stddef.h>
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


#include <lexer/matcher_fsm.h>
#include <lexer/matcher.h>

enum {
    LEX_STATE_DEFAULT,
    S_LL_RULES,
    S_LL_STATE,
    S_GG_RULES,
    S_MATCH_BRACE,
    S_COMMENT,
};


/************************************ TOP ***************************************/


struct LexerTextBuffer
{
    int32_t counter;
    uint32_t s;
    uint32_t n;
    char* buffer;
    TokenPosition start_position;
};

static __thread struct LexerTextBuffer brace_buffer = {0};

static inline void ll_add_to_brace(const char* lex_text, uint32_t len);
static inline void ll_match_brace(const TokenPosition* start_position);



/********************************* DESTRUCTORS **********************************/

static void
action_destructor(NeoastUnion* self) {  free(self->action.string); 
}

static void
file_destructor(NeoastUnion* self) {  file_free(self->file); 
}

static void
g_rule_destructor(NeoastUnion* self) {  grammar_rule_multi_free(self->g_rule); 
}

static void
g_single_rule_destructor(NeoastUnion* self) {  grammar_rule_single_free(self->g_single_rule); 
}

static void
identifier_destructor(NeoastUnion* self) {  free(self->identifier); 
}

static void
key_val_destructor(NeoastUnion* self) {  key_val_free(self->key_val); 
}

static void
l_rule_destructor(NeoastUnion* self) {  lexer_rule_free(self->l_rule); 
}

static void
token_destructor(NeoastUnion* self) {  tokens_free(self->token); 
}


// Destructor table
static const
parser_destructor neoast_token_destructors[] = {
    NULL, // EOF
    NULL, // LL
    NULL, // GG
    NULL, // OPTION
    NULL, // START
    NULL, // UNION
    NULL, // TYPE
    NULL, // LEFT
    NULL, // RIGHT
    NULL, // TOP
    NULL, // INCLUDE
    NULL, // END_STATE
    (parser_destructor) key_val_destructor, // MACRO
    NULL, // BOTTOM
    NULL, // TOKEN
    NULL, // DESTRUCTOR
    NULL, // ASCII
    (parser_destructor) identifier_destructor, // LITERAL
    (parser_destructor) action_destructor, // ACTION
    (parser_destructor) identifier_destructor, // IDENTIFIER
    (parser_destructor) identifier_destructor, // LEX_STATE
    NULL, // ASCII_CHAR_0x3C
    NULL, // ASCII_CHAR_0x3E
    NULL, // ASCII_CHAR_0x3A
    NULL, // ASCII_CHAR_0x7C
    NULL, // ASCII_CHAR_0x3B
    NULL, // ASCII_CHAR_0x3D
    (parser_destructor) g_single_rule_destructor, // single_grammar
    (parser_destructor) g_single_rule_destructor, // multi_grammar
    (parser_destructor) g_rule_destructor, // grammar
    (parser_destructor) g_rule_destructor, // grammars
    (parser_destructor) key_val_destructor, // header
    (parser_destructor) key_val_destructor, // pair
    (parser_destructor) l_rule_destructor, // lexer_rules
    (parser_destructor) l_rule_destructor, // lexer_rules_state
    (parser_destructor) l_rule_destructor, // lexer_rule
    (parser_destructor) token_destructor, // tokens
    (parser_destructor) token_destructor, // token
    (parser_destructor) file_destructor, // program
    (parser_destructor) file_destructor, // TOK_AUGMENT

};

/************************************ LEXER *************************************/
static void LEX_STATE_DEFAULT_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '{') goto S53;
  if ('a' <= c1 && c1 <= 'z') goto S47;
  if (c1 == '_') goto S47;
  if ('A' <= c1 && c1 <= 'Z') goto S47;
  if (c1 == '>') goto S41;
  if (c1 == '=') goto S25;
  if (c1 == '<') goto S39;
  if (c1 == '/') goto S16;
  if (c1 == '+') goto S43;
  if (c1 == '\'') goto S22;
  if (c1 == '%') goto S28;
  if (c1 == '"') goto S19;
  if (c1 == ' ') goto S55;
  if (c1 == '\r') goto S55;
  if ('\t' <= c1 && c1 <= '\n') goto S55;
  return FSM_HALT(m, c1);

S16:
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S60;
  if (c1 == '*') goto S64;
  return FSM_HALT(m, c1);

S19:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S68;
  if (c1 == '"') goto S66;
  if (0 <= c1) goto S19;
  return FSM_HALT(m, c1);

S22:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S73;
  if (' ' <= c1 && c1 <= '~') goto S71;
  return FSM_HALT(m, c1);

S25:
  FSM_TAKE(m, 8, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '=') goto S77;
  return FSM_HALT(m, c1);

S28:
  c1 = FSM_CHAR(m);
  if (c1 == 'u') goto S88;
  if (c1 == 't') goto S83;
  if (c1 == 's') goto S86;
  if (c1 == 'r') goto S92;
  if (c1 == 'o') goto S81;
  if (c1 == 'l') goto S90;
  if (c1 == 'i') goto S94;
  if (c1 == 'd') goto S98;
  if (c1 == 'b') goto S96;
  if (c1 == '%') goto S79;
  return FSM_HALT(m, c1);

S39:
  FSM_TAKE(m, 9, EOF);
  return FSM_HALT(m, CONST_UNK);

S41:
  FSM_TAKE(m, 10, EOF);
  return FSM_HALT(m, CONST_UNK);

S43:
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S100;
  if (c1 == '_') goto S100;
  if ('A' <= c1 && c1 <= 'Z') goto S100;
  return FSM_HALT(m, c1);

S47:
  FSM_TAKE(m, 23, EOF);
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S47;
  if (c1 == '_') goto S47;
  if ('A' <= c1 && c1 <= 'Z') goto S47;
  if ('0' <= c1 && c1 <= '9') goto S47;
  return FSM_HALT(m, c1);

S53:
  FSM_TAKE(m, 24, EOF);
  return FSM_HALT(m, CONST_UNK);

S55:
  FSM_TAKE(m, 1, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S55;
  if (c1 == '\r') goto S55;
  if ('\t' <= c1 && c1 <= '\n') goto S55;
  return FSM_HALT(m, c1);

S60:
  FSM_TAKE(m, 2, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S60;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S60;
  return FSM_HALT(m, c1);

S64:
  FSM_TAKE(m, 3, EOF);
  return FSM_HALT(m, CONST_UNK);

S66:
  FSM_TAKE(m, 4, EOF);
  return FSM_HALT(m, CONST_UNK);

S68:
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S19;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S19;
  return FSM_HALT(m, c1);

S71:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S106;
  return FSM_HALT(m, c1);

S73:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S108;
  if ('\v' <= c1) goto S71;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S71;
  return FSM_HALT(m, c1);

S77:
  FSM_TAKE(m, 6, EOF);
  return FSM_HALT(m, CONST_UNK);

S79:
  FSM_TAKE(m, 7, EOF);
  return FSM_HALT(m, CONST_UNK);

S81:
  c1 = FSM_CHAR(m);
  if (c1 == 'p') goto S111;
  return FSM_HALT(m, c1);

S83:
  c1 = FSM_CHAR(m);
  if (c1 == 'y') goto S116;
  if (c1 == 'o') goto S113;
  return FSM_HALT(m, c1);

S86:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S118;
  return FSM_HALT(m, c1);

S88:
  c1 = FSM_CHAR(m);
  if (c1 == 'n') goto S120;
  return FSM_HALT(m, c1);

S90:
  c1 = FSM_CHAR(m);
  if (c1 == 'e') goto S122;
  return FSM_HALT(m, c1);

S92:
  c1 = FSM_CHAR(m);
  if (c1 == 'i') goto S124;
  return FSM_HALT(m, c1);

S94:
  c1 = FSM_CHAR(m);
  if (c1 == 'n') goto S126;
  return FSM_HALT(m, c1);

S96:
  c1 = FSM_CHAR(m);
  if (c1 == 'o') goto S128;
  return FSM_HALT(m, c1);

S98:
  c1 = FSM_CHAR(m);
  if (c1 == 'e') goto S130;
  return FSM_HALT(m, c1);

S100:
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S100;
  if (c1 == '_') goto S100;
  if ('A' <= c1 && c1 <= 'Z') goto S100;
  if ('0' <= c1 && c1 <= '9') goto S100;
  if (c1 == ' ') goto S132;
  return FSM_HALT(m, c1);

S106:
  FSM_TAKE(m, 5, EOF);
  return FSM_HALT(m, CONST_UNK);

S108:
  FSM_TAKE(m, 5, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S106;
  return FSM_HALT(m, c1);

S111:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S136;
  return FSM_HALT(m, c1);

S113:
  c1 = FSM_CHAR(m);
  if (c1 == 'p') goto S140;
  if (c1 == 'k') goto S138;
  return FSM_HALT(m, c1);

S116:
  c1 = FSM_CHAR(m);
  if (c1 == 'p') goto S142;
  return FSM_HALT(m, c1);

S118:
  c1 = FSM_CHAR(m);
  if (c1 == 'a') goto S144;
  return FSM_HALT(m, c1);

S120:
  c1 = FSM_CHAR(m);
  if (c1 == 'i') goto S146;
  return FSM_HALT(m, c1);

S122:
  c1 = FSM_CHAR(m);
  if (c1 == 'f') goto S148;
  return FSM_HALT(m, c1);

S124:
  c1 = FSM_CHAR(m);
  if (c1 == 'g') goto S150;
  return FSM_HALT(m, c1);

S126:
  c1 = FSM_CHAR(m);
  if (c1 == 'c') goto S152;
  return FSM_HALT(m, c1);

S128:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S154;
  return FSM_HALT(m, c1);

S130:
  c1 = FSM_CHAR(m);
  if (c1 == 's') goto S156;
  return FSM_HALT(m, c1);

S132:
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S158;
  if (c1 == '\n') goto S132;
  if ('\t' <= c1 && c1 <= '\r') goto S158;
  if (0 <= c1) goto S163;
  return FSM_HALT(m, c1);

S136:
  c1 = FSM_CHAR(m);
  if (c1 == 'i') goto S167;
  return FSM_HALT(m, c1);

S138:
  c1 = FSM_CHAR(m);
  if (c1 == 'e') goto S169;
  return FSM_HALT(m, c1);

S140:
  FSM_TAKE(m, 18, EOF);
  return FSM_HALT(m, CONST_UNK);

S142:
  c1 = FSM_CHAR(m);
  if (c1 == 'e') goto S171;
  return FSM_HALT(m, c1);

S144:
  c1 = FSM_CHAR(m);
  if (c1 == 'r') goto S173;
  return FSM_HALT(m, c1);

S146:
  c1 = FSM_CHAR(m);
  if (c1 == 'o') goto S175;
  return FSM_HALT(m, c1);

S148:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S177;
  return FSM_HALT(m, c1);

S150:
  c1 = FSM_CHAR(m);
  if (c1 == 'h') goto S179;
  return FSM_HALT(m, c1);

S152:
  c1 = FSM_CHAR(m);
  if (c1 == 'l') goto S181;
  return FSM_HALT(m, c1);

S154:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S183;
  return FSM_HALT(m, c1);

S156:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S185;
  return FSM_HALT(m, c1);

S158:
  FSM_TAKE(m, 22, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S158;
  if (c1 == '\n') goto S132;
  if ('\t' <= c1 && c1 <= '\r') goto S158;
  if (0 <= c1) goto S163;
  return FSM_HALT(m, c1);

S163:
  FSM_TAKE(m, 22, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S163;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S163;
  return FSM_HALT(m, c1);

S167:
  c1 = FSM_CHAR(m);
  if (c1 == 'o') goto S187;
  return FSM_HALT(m, c1);

S169:
  c1 = FSM_CHAR(m);
  if (c1 == 'n') goto S189;
  return FSM_HALT(m, c1);

S171:
  FSM_TAKE(m, 15, EOF);
  return FSM_HALT(m, CONST_UNK);

S173:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S191;
  return FSM_HALT(m, c1);

S175:
  c1 = FSM_CHAR(m);
  if (c1 == 'n') goto S193;
  return FSM_HALT(m, c1);

S177:
  FSM_TAKE(m, 16, EOF);
  return FSM_HALT(m, CONST_UNK);

S179:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S195;
  return FSM_HALT(m, c1);

S181:
  c1 = FSM_CHAR(m);
  if (c1 == 'u') goto S197;
  return FSM_HALT(m, c1);

S183:
  c1 = FSM_CHAR(m);
  if (c1 == 'o') goto S199;
  return FSM_HALT(m, c1);

S185:
  c1 = FSM_CHAR(m);
  if (c1 == 'r') goto S201;
  return FSM_HALT(m, c1);

S187:
  c1 = FSM_CHAR(m);
  if (c1 == 'n') goto S203;
  return FSM_HALT(m, c1);

S189:
  FSM_TAKE(m, 12, EOF);
  return FSM_HALT(m, CONST_UNK);

S191:
  FSM_TAKE(m, 13, EOF);
  return FSM_HALT(m, CONST_UNK);

S193:
  FSM_TAKE(m, 14, EOF);
  return FSM_HALT(m, CONST_UNK);

S195:
  FSM_TAKE(m, 17, EOF);
  return FSM_HALT(m, CONST_UNK);

S197:
  c1 = FSM_CHAR(m);
  if (c1 == 'd') goto S205;
  return FSM_HALT(m, c1);

S199:
  c1 = FSM_CHAR(m);
  if (c1 == 'm') goto S207;
  return FSM_HALT(m, c1);

S201:
  c1 = FSM_CHAR(m);
  if (c1 == 'u') goto S209;
  return FSM_HALT(m, c1);

S203:
  FSM_TAKE(m, 11, EOF);
  return FSM_HALT(m, CONST_UNK);

S205:
  c1 = FSM_CHAR(m);
  if (c1 == 'e') goto S211;
  return FSM_HALT(m, c1);

S207:
  FSM_TAKE(m, 20, EOF);
  return FSM_HALT(m, CONST_UNK);

S209:
  c1 = FSM_CHAR(m);
  if (c1 == 'c') goto S213;
  return FSM_HALT(m, c1);

S211:
  FSM_TAKE(m, 19, EOF);
  return FSM_HALT(m, CONST_UNK);

S213:
  c1 = FSM_CHAR(m);
  if (c1 == 't') goto S215;
  return FSM_HALT(m, c1);

S215:
  c1 = FSM_CHAR(m);
  if (c1 == 'o') goto S217;
  return FSM_HALT(m, c1);

S217:
  c1 = FSM_CHAR(m);
  if (c1 == 'r') goto S219;
  return FSM_HALT(m, c1);

S219:
  FSM_TAKE(m, 21, EOF);
  return FSM_HALT(m, CONST_UNK);
}


static void S_LL_RULES_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '{') goto S27;
  if (c1 == '=') goto S18;
  if (c1 == '<') goto S20;
  if (c1 == '/') goto S15;
  if (c1 == '"') goto S24;
  if (c1 == ' ') goto S29;
  if (c1 == '\r') goto S29;
  if (c1 == '\n') goto S10;
  if (c1 == '\t') goto S29;
  return FSM_HALT(m, c1);

S10:
  FSM_TAKE(m, 1, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S29;
  if (c1 == '\r') goto S29;
  if ('\t' <= c1 && c1 <= '\n') goto S29;
  return FSM_HALT(m, c1);

S15:
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S34;
  if (c1 == '*') goto S38;
  return FSM_HALT(m, c1);

S18:
  c1 = FSM_CHAR(m);
  if (c1 == '=') goto S40;
  return FSM_HALT(m, c1);

S20:
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S42;
  if (c1 == '_') goto S42;
  if ('A' <= c1 && c1 <= 'Z') goto S42;
  return FSM_HALT(m, c1);

S24:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S50;
  if (c1 == '"') goto S48;
  if (0 <= c1) goto S24;
  return FSM_HALT(m, c1);

S27:
  FSM_TAKE(m, 8, EOF);
  return FSM_HALT(m, CONST_UNK);

S29:
  FSM_TAKE(m, 2, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S29;
  if (c1 == '\r') goto S29;
  if ('\t' <= c1 && c1 <= '\n') goto S29;
  return FSM_HALT(m, c1);

S34:
  FSM_TAKE(m, 3, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S34;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S34;
  return FSM_HALT(m, c1);

S38:
  FSM_TAKE(m, 4, EOF);
  return FSM_HALT(m, CONST_UNK);

S40:
  FSM_TAKE(m, 5, EOF);
  return FSM_HALT(m, CONST_UNK);

S42:
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S42;
  if (c1 == '_') goto S42;
  if ('A' <= c1 && c1 <= 'Z') goto S42;
  if (c1 == '>') goto S53;
  if ('0' <= c1 && c1 <= '9') goto S42;
  return FSM_HALT(m, c1);

S48:
  FSM_TAKE(m, 7, EOF);
  return FSM_HALT(m, CONST_UNK);

S50:
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S24;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S24;
  return FSM_HALT(m, c1);

S53:
  c1 = FSM_CHAR(m);
  if (c1 == '{') goto S57;
  if (c1 == ' ') goto S53;
  if ('\t' <= c1 && c1 <= '\r') goto S53;
  return FSM_HALT(m, c1);

S57:
  FSM_TAKE(m, 6, EOF);
  return FSM_HALT(m, CONST_UNK);
}


static void S_LL_STATE_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '}') goto S16;
  if (c1 == '{') goto S14;
  if (c1 == '/') goto S8;
  if (c1 == '"') goto S11;
  if (c1 == ' ') goto S18;
  if (c1 == '\r') goto S18;
  if ('\t' <= c1 && c1 <= '\n') goto S18;
  return FSM_HALT(m, c1);

S8:
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S23;
  if (c1 == '*') goto S27;
  return FSM_HALT(m, c1);

S11:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S31;
  if (c1 == '"') goto S29;
  if (0 <= c1) goto S11;
  return FSM_HALT(m, c1);

S14:
  FSM_TAKE(m, 5, EOF);
  return FSM_HALT(m, CONST_UNK);

S16:
  FSM_TAKE(m, 6, EOF);
  return FSM_HALT(m, CONST_UNK);

S18:
  FSM_TAKE(m, 1, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S18;
  if (c1 == '\r') goto S18;
  if ('\t' <= c1 && c1 <= '\n') goto S18;
  return FSM_HALT(m, c1);

S23:
  FSM_TAKE(m, 2, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S23;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S23;
  return FSM_HALT(m, c1);

S27:
  FSM_TAKE(m, 3, EOF);
  return FSM_HALT(m, CONST_UNK);

S29:
  FSM_TAKE(m, 4, EOF);
  return FSM_HALT(m, CONST_UNK);

S31:
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S11;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S11;
  return FSM_HALT(m, c1);
}


static void S_GG_RULES_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '|') goto S30;
  if (c1 == '{') goto S34;
  if ('a' <= c1 && c1 <= 'z') goto S22;
  if (c1 == '_') goto S22;
  if ('A' <= c1 && c1 <= 'Z') goto S22;
  if (c1 == ';') goto S32;
  if (c1 == ':') goto S28;
  if (c1 == '/') goto S14;
  if (c1 == '\'') goto S17;
  if (c1 == '%') goto S20;
  if (c1 == ' ') goto S36;
  if (c1 == '\r') goto S36;
  if ('\t' <= c1 && c1 <= '\n') goto S36;
  return FSM_HALT(m, c1);

S14:
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S41;
  if (c1 == '*') goto S45;
  return FSM_HALT(m, c1);

S17:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S49;
  if (' ' <= c1 && c1 <= '~') goto S47;
  return FSM_HALT(m, c1);

S20:
  c1 = FSM_CHAR(m);
  if (c1 == '%') goto S53;
  return FSM_HALT(m, c1);

S22:
  FSM_TAKE(m, 6, EOF);
  c1 = FSM_CHAR(m);
  if ('a' <= c1 && c1 <= 'z') goto S22;
  if (c1 == '_') goto S22;
  if ('A' <= c1 && c1 <= 'Z') goto S22;
  if ('0' <= c1 && c1 <= '9') goto S22;
  return FSM_HALT(m, c1);

S28:
  FSM_TAKE(m, 7, EOF);
  return FSM_HALT(m, CONST_UNK);

S30:
  FSM_TAKE(m, 8, EOF);
  return FSM_HALT(m, CONST_UNK);

S32:
  FSM_TAKE(m, 9, EOF);
  return FSM_HALT(m, CONST_UNK);

S34:
  FSM_TAKE(m, 10, EOF);
  return FSM_HALT(m, CONST_UNK);

S36:
  FSM_TAKE(m, 1, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == ' ') goto S36;
  if (c1 == '\r') goto S36;
  if ('\t' <= c1 && c1 <= '\n') goto S36;
  return FSM_HALT(m, c1);

S41:
  FSM_TAKE(m, 2, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S41;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S41;
  return FSM_HALT(m, c1);

S45:
  FSM_TAKE(m, 4, EOF);
  return FSM_HALT(m, CONST_UNK);

S47:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S55;
  return FSM_HALT(m, c1);

S49:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S57;
  if ('\v' <= c1) goto S47;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S47;
  return FSM_HALT(m, c1);

S53:
  FSM_TAKE(m, 5, EOF);
  return FSM_HALT(m, CONST_UNK);

S55:
  FSM_TAKE(m, 3, EOF);
  return FSM_HALT(m, CONST_UNK);

S57:
  FSM_TAKE(m, 3, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S55;
  return FSM_HALT(m, c1);
}


static void S_MATCH_BRACE_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '}') goto S18;
  if (c1 == '{') goto S16;
  if (c1 == '/') goto S6;
  if (c1 == '\'') goto S10;
  if (c1 == '"') goto S13;
  if (0 <= c1) goto S20;
  return FSM_HALT(m, c1);

S6:
  FSM_TAKE(m, 6, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S30;
  if (c1 == '*') goto S28;
  return FSM_HALT(m, c1);

S10:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S36;
  if (' ' <= c1 && c1 <= '~') goto S34;
  return FSM_HALT(m, c1);

S13:
  c1 = FSM_CHAR(m);
  if (c1 == '\\') goto S42;
  if (c1 == '"') goto S40;
  if (0 <= c1) goto S13;
  return FSM_HALT(m, c1);

S16:
  FSM_TAKE(m, 7, EOF);
  return FSM_HALT(m, CONST_UNK);

S18:
  FSM_TAKE(m, 8, EOF);
  return FSM_HALT(m, CONST_UNK);

S20:
  FSM_TAKE(m, 5, EOF);
  c1 = FSM_CHAR(m);
  if ('~' <= c1) goto S20;
  if (c1 == '|') goto S20;
  if ('0' <= c1 && c1 <= 'z') goto S20;
  if ('(' <= c1 && c1 <= '.') goto S20;
  if ('#' <= c1 && c1 <= '&') goto S20;
  if ('"' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '!') goto S20;
  return FSM_HALT(m, c1);

S28:
  FSM_TAKE(m, 1, EOF);
  return FSM_HALT(m, CONST_UNK);

S30:
  FSM_TAKE(m, 2, EOF);
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S30;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S30;
  return FSM_HALT(m, c1);

S34:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S45;
  return FSM_HALT(m, c1);

S36:
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S47;
  if ('\v' <= c1) goto S34;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S34;
  return FSM_HALT(m, c1);

S40:
  FSM_TAKE(m, 4, EOF);
  return FSM_HALT(m, CONST_UNK);

S42:
  c1 = FSM_CHAR(m);
  if ('\v' <= c1) goto S13;
  if ('\n' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= '\t') goto S13;
  return FSM_HALT(m, c1);

S45:
  FSM_TAKE(m, 3, EOF);
  return FSM_HALT(m, CONST_UNK);

S47:
  FSM_TAKE(m, 3, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '\'') goto S45;
  return FSM_HALT(m, c1);
}


static void S_COMMENT_FSM(NeoastMatcher* m)
{
  int c0, c1 = 0;
  FSM_INIT(m, &c1);

S0:
  FSM_FIND(m);
  c1 = FSM_CHAR(m);
  if (c1 == '*') goto S2;
  if (0 <= c1) goto S5;
  return FSM_HALT(m, c1);

S2:
  FSM_TAKE(m, 3, EOF);
  c1 = FSM_CHAR(m);
  if (c1 == '/') goto S9;
  return FSM_HALT(m, c1);

S5:
  FSM_TAKE(m, 1, EOF);
  c1 = FSM_CHAR(m);
  if ('+' <= c1) goto S5;
  if ('*' <= c1) return FSM_HALT(m, c1);
  if (0 <= c1 && c1 <= ')') goto S5;
  return FSM_HALT(m, c1);

S9:
  FSM_TAKE(m, 2, EOF);
  return FSM_HALT(m, CONST_UNK);
}


static int neoast_lexer_next(NeoastMatcher* self__, NeoastValue* destination__, void* context__)
{
#define yyval (&(destination__->value))
#define yystate (self__->lexing_state)
#define yypush(state) NEOAST_STACK_PUSH(yystate, (state))
#define yypop() NEOAST_STACK_POP(yystate)
#define yyposition (&(destination__->position))
#define yycontext (context__)
#define yylen (self__->len_)

    while (!matcher_at_end(self__))
    {
        switch (NEOAST_STACK_PEEK(yystate))
        {
        case LEX_STATE_DEFAULT:
        {
            size_t neoast_tok___ = matcher_scan(self__, LEX_STATE_DEFAULT_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "LEX_STATE_DEFAULT"); return -1; }
                else return 0;
            case 1:
 {  }
                break;
            case 2:
 {  }
                break;
            case 3:
 { yypush(S_COMMENT); }
                break;
            case 4:
 { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
                break;
            case 5:
 { yyval->ascii = yytext[1]; return ASCII; }
                break;
            case 6:
 { yypush(S_LL_RULES); return LL; }
                break;
            case 7:
 { yypush(S_GG_RULES); return GG; }
                break;
            case 8:
 { return '='; }
                break;
            case 9:
 { return '<'; }
                break;
            case 10:
 { return '>'; }
                break;
            case 11:
 { return OPTION; }
                break;
            case 12:
 { return TOKEN; }
                break;
            case 13:
 { return START; }
                break;
            case 14:
 { return UNION; }
                break;
            case 15:
 { return TYPE; }
                break;
            case 16:
 { return LEFT; }
                break;
            case 17:
 { return RIGHT; }
                break;
            case 18:
 { return TOP; }
                break;
            case 19:
 { return INCLUDE; }
                break;
            case 20:
 { return BOTTOM; }
                break;
            case 21:
 { return DESTRUCTOR; }
                break;
            case 22:
 {
                        // Find the white space delimiter
                        const char* split = strchr(yytext + 1, ' ');
                        assert(split);

                        char* key = strndup(yytext + 1, split - yytext - 1);

                        // Find the start of the regex rule
                        while (*split == ' ') split++;

                        char* value = strdup(split);
                        yyval->key_val = key_val_build(yyposition, KEY_VAL_MACRO, key, value);
                        return MACRO;
                    }
                break;
            case 23:
 { yyval->identifier = strdup(yytext); return IDENTIFIER; }
                break;
            case 24:
 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
                break;
            }

        }
        break;
        case S_LL_RULES:
        {
            size_t neoast_tok___ = matcher_scan(self__, S_LL_RULES_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "S_LL_RULES"); return -1; }
                else return 0;
            case 1:
 {  }
                break;
            case 2:
 {  }
                break;
            case 3:
 {  }
                break;
            case 4:
 { yypush(S_COMMENT); }
                break;
            case 5:
 { yypop(); return LL; }
                break;
            case 6:
 {
                        const char* start_ptr = strchr(yytext, '<');
                        const char* end_ptr = strchr(yytext, '>');
                        yyval->identifier = strndup(start_ptr + 1, end_ptr - start_ptr - 1);
                        yypush(S_LL_STATE);

                        return LEX_STATE;
                    }
                break;
            case 7:
 { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
                break;
            case 8:
 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
                break;
            }

        }
        break;
        case S_LL_STATE:
        {
            size_t neoast_tok___ = matcher_scan(self__, S_LL_STATE_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "S_LL_STATE"); return -1; }
                else return 0;
            case 1:
 {  }
                break;
            case 2:
 {  }
                break;
            case 3:
 { yypush(S_COMMENT); }
                break;
            case 4:
 { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
                break;
            case 5:
 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
                break;
            case 6:
 { yypop(); return END_STATE; }
                break;
            }

        }
        break;
        case S_GG_RULES:
        {
            size_t neoast_tok___ = matcher_scan(self__, S_GG_RULES_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "S_GG_RULES"); return -1; }
                else return 0;
            case 1:
 {  }
                break;
            case 2:
 {  }
                break;
            case 3:
 { yyval->ascii = yytext[1]; return ASCII; }
                break;
            case 4:
 { yypush(S_COMMENT); }
                break;
            case 5:
 { yypop(); return GG; }
                break;
            case 6:
 { yyval->identifier = strdup(yytext); return IDENTIFIER; }
                break;
            case 7:
 { return ':'; }
                break;
            case 8:
 { return '|'; }
                break;
            case 9:
 { return ';'; }
                break;
            case 10:
 { yypush(S_MATCH_BRACE); ll_match_brace(yyposition); }
                break;
            }

        }
        break;
        case S_MATCH_BRACE:
        {
            size_t neoast_tok___ = matcher_scan(self__, S_MATCH_BRACE_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "S_MATCH_BRACE"); return -1; }
                else return 0;
            case 1:
 { yypush(S_COMMENT); }
                break;
            case 2:
 { ll_add_to_brace(yytext, yylen); }
                break;
            case 3:
 { ll_add_to_brace(yytext, yylen); }
                break;
            case 4:
 { ll_add_to_brace(yytext, yylen); }
                break;
            case 5:
 { ll_add_to_brace(yytext, yylen); }
                break;
            case 6:
 { ll_add_to_brace(yytext, yylen); }
                break;
            case 7:
 { brace_buffer.counter++; brace_buffer.buffer[brace_buffer.n++] = '{'; }
                break;
            case 8:
 {
                        brace_buffer.counter--;
                        if (brace_buffer.counter <= 0)
                        {
                            // This is the outer-most brace
                            brace_buffer.buffer[brace_buffer.n++] = 0;
                            yypop();
                            yyval->action.string = strndup(brace_buffer.buffer, brace_buffer.n);
                            yyval->action.position = brace_buffer.start_position;
                            free(brace_buffer.buffer);
                            brace_buffer.buffer = NULL;
                            return ACTION;
                        }
                        else
                        {
                            brace_buffer.buffer[brace_buffer.n++] = '}';
                        }
                    }
                break;
            }

        }
        break;
        case S_COMMENT:
        {
            size_t neoast_tok___ = matcher_scan(self__, S_COMMENT_FSM);
            const char* yytext = matcher_text(self__);
            yyposition->line = matcher_lineno(self__);
            yyposition->col = matcher_columno(self__);
            yyposition->len = matcher_size(self__);

            switch(neoast_tok___)
            {
            default:
            case 0:
                if (!matcher_at_end(self__)) { lexing_error_cb(yycontext, yytext, yyposition, "S_COMMENT"); return -1; }
                else return 0;
            case 1:
 {  }
                break;
            case 2:
 { yypop(); }
                break;
            case 3:
 { }
                break;
            }

        }
        break;
    }}

    // EOF
    return 0;
#undef yyval
#undef yystate
#undef yypush
#undef yypop
#undef yyposition
#undef yycontext
#undef yylen
}


/*********************************** GRAMMAR ************************************/
static void neoast_reduce_handler(uint32_t reduce_id__, NeoastValue* dest__, NeoastValue* args__, void* context__)
{
#define yycontext (context__)
    switch(reduce_id__)
    {
        case 1:
 dest__->value.g_rule = declare_grammar(((const TokenPosition*)&args__[0].position), args__[0].value.identifier, args__[2].value.g_single_rule); 
            break;
        case 2:
 dest__->value.g_rule = args__[0].value.g_rule; 
            break;
        case 3:
 dest__->value.g_rule = args__[0].value.g_rule; dest__->value.g_rule->next = args__[1].value.g_rule; 
            break;
        case 4:
 dest__->value.key_val = args__[0].value.key_val; 
            break;
        case 5:
 dest__->value.key_val = args__[0].value.key_val->back ? args__[0].value.key_val->back : args__[0].value.key_val; args__[0].value.key_val->next = args__[1].value.key_val; 
            break;
        case 6:
 dest__->value.l_rule = declare_lexer_rule(&args__[1].value.action.position, args__[0].value.identifier, args__[1].value.action.string); 
            break;
        case 7:
 dest__->value.l_rule = declare_state_rule(((const TokenPosition*)&args__[0].position), args__[0].value.identifier, args__[1].value.l_rule); 
            break;
        case 8:
 dest__->value.l_rule = args__[0].value.l_rule; 
            break;
        case 9:
 dest__->value.l_rule = args__[0].value.l_rule; dest__->value.l_rule->next = args__[1].value.l_rule; 
            break;
        case 10:
 dest__->value.l_rule = args__[0].value.l_rule; 
            break;
        case 11:
 dest__->value.l_rule = args__[0].value.l_rule; dest__->value.l_rule->next = args__[1].value.l_rule; 
            break;
        case 12:
 dest__->value.g_single_rule = args__[0].value.g_single_rule; 
            break;
        case 13:
 dest__->value.g_single_rule = args__[0].value.g_single_rule; dest__->value.g_single_rule->next = args__[2].value.g_single_rule; 
            break;
        case 14:
 dest__->value.key_val = declare_option(((const TokenPosition*)&args__[1].position), args__[1].value.identifier, args__[3].value.identifier); 
            break;
        case 15:
 dest__->value.key_val = declare_tokens(args__[1].value.token); 
            break;
        case 16:
 dest__->value.key_val = declare_types(NULL, args__[1].value.token); 
            break;
        case 17:
 dest__->value.key_val = declare_start(((const TokenPosition*)&args__[0].position), args__[2].value.identifier, args__[4].value.identifier); 
            break;
        case 18:
 dest__->value.key_val = declare_typed_tokens(args__[2].value.identifier, args__[4].value.token); 
            break;
        case 19:
 dest__->value.key_val = declare_types(args__[2].value.identifier, args__[4].value.token); 
            break;
        case 20:
 dest__->value.key_val = declare_destructor(&args__[4].value.action.position, args__[2].value.identifier, args__[4].value.action.string); 
            break;
        case 21:
 dest__->value.key_val = declare_right(args__[1].value.token); 
            break;
        case 22:
 dest__->value.key_val = declare_left(args__[1].value.token); 
            break;
        case 23:
 dest__->value.key_val = declare_top(&args__[1].value.action.position, args__[1].value.action.string); 
            break;
        case 24:
 dest__->value.key_val = declare_include(&args__[1].value.action.position, args__[1].value.action.string); 
            break;
        case 25:
 dest__->value.key_val = declare_bottom(&args__[1].value.action.position, args__[1].value.action.string); 
            break;
        case 26:
 dest__->value.key_val = declare_union(&args__[1].value.action.position, args__[1].value.action.string); 
            break;
        case 27:
 dest__->value.key_val = args__[0].value.key_val; 
            break;
        case 28:
 dest__->value.file = declare_file(args__[0].value.key_val, args__[2].value.l_rule, args__[5].value.g_rule); 
            break;
        case 29:
 dest__->value.g_single_rule = declare_single_grammar(&args__[1].value.action.position, args__[0].value.token, args__[1].value.action.string); 
            break;
        case 30:
 dest__->value.g_single_rule = declare_single_grammar(&args__[0].value.action.position, NULL, args__[0].value.action.string); 
            break;
        case 31:
 dest__->value.g_single_rule = declare_single_grammar(((const TokenPosition*)&args__[0].position), args__[0].value.token, calloc(1, 1)); 
            break;
        case 32:
 dest__->value.token = build_token(((const TokenPosition*)&args__[0].position), args__[0].value.identifier); 
            break;
        case 33:
 dest__->value.token = build_token_ascii(((const TokenPosition*)&args__[0].position), args__[0].value.ascii); 
            break;
        case 34:
 dest__->value.token = args__[0].value.token; 
            break;
        case 35:
 dest__->value.token = args__[0].value.token; dest__->value.token->next = args__[1].value.token; 
            break;
    default:
        *dest__ = args__[0];
        break;
    }
#undef yycontext
}
static const
unsigned int grammar_token_table[] = {
        /* TOK_AUGMENT */
        /* ACCEPT */ program,

        /* grammar */
        /* R01 */ IDENTIFIER, ASCII_CHAR_0x3A, multi_grammar, ASCII_CHAR_0x3B,

        /* grammars */
        /* R02 */ grammar,
        /* R03 */ grammar, grammars,

        /* header */
        /* R04 */ pair,
        /* R05 */ pair, header,

        /* lexer_rule */
        /* R06 */ LITERAL, ACTION,
        /* R07 */ LEX_STATE, lexer_rules_state, END_STATE,

        /* lexer_rules */
        /* R08 */ lexer_rule,
        /* R09 */ lexer_rule, lexer_rules,

        /* lexer_rules_state */
        /* R10 */ lexer_rule,
        /* R11 */ lexer_rule, lexer_rules_state,

        /* multi_grammar */
        /* R12 */ single_grammar,
        /* R13 */ single_grammar, ASCII_CHAR_0x7C, multi_grammar,

        /* pair */
        /* R14 */ OPTION, IDENTIFIER, ASCII_CHAR_0x3D, LITERAL,
        /* R15 */ TOKEN, tokens,
        /* R16 */ TYPE, tokens,
        /* R17 */ START, ASCII_CHAR_0x3C, IDENTIFIER, ASCII_CHAR_0x3E, IDENTIFIER,
        /* R18 */ TOKEN, ASCII_CHAR_0x3C, IDENTIFIER, ASCII_CHAR_0x3E, tokens,
        /* R19 */ TYPE, ASCII_CHAR_0x3C, IDENTIFIER, ASCII_CHAR_0x3E, tokens,
        /* R20 */ DESTRUCTOR, ASCII_CHAR_0x3C, IDENTIFIER, ASCII_CHAR_0x3E, ACTION,
        /* R21 */ RIGHT, tokens,
        /* R22 */ LEFT, tokens,
        /* R23 */ TOP, ACTION,
        /* R24 */ INCLUDE, ACTION,
        /* R25 */ BOTTOM, ACTION,
        /* R26 */ UNION, ACTION,
        /* R27 */ MACRO,

        /* program */
        /* R28 */ header, LL, lexer_rules, LL, GG, grammars, GG,

        /* single_grammar */
        /* R29 */ tokens, ACTION,
        /* R30 */ ACTION,
        /* R31 */ tokens,

        /* token */
        /* R32 */ IDENTIFIER,
        /* R33 */ ASCII,

        /* tokens */
        /* R34 */ token,
        /* R35 */ token, tokens,

};

static const
GrammarRule neoast_grammar_rules[] = {
        {.token=TOK_AUGMENT, .tok_n=1, .grammar=&grammar_token_table[0]},
        {.token=grammar, .tok_n=4, .grammar=&grammar_token_table[1]},
        {.token=grammars, .tok_n=1, .grammar=&grammar_token_table[5]},
        {.token=grammars, .tok_n=2, .grammar=&grammar_token_table[6]},
        {.token=header, .tok_n=1, .grammar=&grammar_token_table[8]},
        {.token=header, .tok_n=2, .grammar=&grammar_token_table[9]},
        {.token=lexer_rule, .tok_n=2, .grammar=&grammar_token_table[11]},
        {.token=lexer_rule, .tok_n=3, .grammar=&grammar_token_table[13]},
        {.token=lexer_rules, .tok_n=1, .grammar=&grammar_token_table[16]},
        {.token=lexer_rules, .tok_n=2, .grammar=&grammar_token_table[17]},
        {.token=lexer_rules_state, .tok_n=1, .grammar=&grammar_token_table[19]},
        {.token=lexer_rules_state, .tok_n=2, .grammar=&grammar_token_table[20]},
        {.token=multi_grammar, .tok_n=1, .grammar=&grammar_token_table[22]},
        {.token=multi_grammar, .tok_n=3, .grammar=&grammar_token_table[23]},
        {.token=pair, .tok_n=4, .grammar=&grammar_token_table[26]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[30]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[32]},
        {.token=pair, .tok_n=5, .grammar=&grammar_token_table[34]},
        {.token=pair, .tok_n=5, .grammar=&grammar_token_table[39]},
        {.token=pair, .tok_n=5, .grammar=&grammar_token_table[44]},
        {.token=pair, .tok_n=5, .grammar=&grammar_token_table[49]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[54]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[56]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[58]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[60]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[62]},
        {.token=pair, .tok_n=2, .grammar=&grammar_token_table[64]},
        {.token=pair, .tok_n=1, .grammar=&grammar_token_table[66]},
        {.token=program, .tok_n=7, .grammar=&grammar_token_table[67]},
        {.token=single_grammar, .tok_n=2, .grammar=&grammar_token_table[74]},
        {.token=single_grammar, .tok_n=1, .grammar=&grammar_token_table[76]},
        {.token=single_grammar, .tok_n=1, .grammar=&grammar_token_table[77]},
        {.token=token, .tok_n=1, .grammar=&grammar_token_table[78]},
        {.token=token, .tok_n=1, .grammar=&grammar_token_table[79]},
        {.token=tokens, .tok_n=1, .grammar=&grammar_token_table[80]},
        {.token=tokens, .tok_n=2, .grammar=&grammar_token_table[81]},
};



/******************************* ASCII MAPPINGS *********************************/
static const
uint32_t neoast_ascii_mappings[NEOAST_ASCII_MAX] = {
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x117, 0x119,
        0x115, 0x11A, 0x116, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x118, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
        0x000, 0x000, 0x000, 0x000, 
};

/***************************** NEOAST DEFINITIONS ********************************/
static const
char* neoast_token_names[] = {
        "EOF",
        "LL",
        "GG",
        "OPTION",
        "START",
        "UNION",
        "TYPE",
        "LEFT",
        "RIGHT",
        "TOP",
        "INCLUDE",
        "END_STATE",
        "MACRO",
        "BOTTOM",
        "TOKEN",
        "DESTRUCTOR",
        "ASCII",
        "LITERAL",
        "ACTION",
        "IDENTIFIER",
        "LEX_STATE",
        "<",
        ">",
        ":",
        "|",
        ";",
        "=",
        "single_grammar",
        "multi_grammar",
        "grammar",
        "grammars",
        "header",
        "pair",
        "lexer_rules",
        "lexer_rules_state",
        "lexer_rule",
        "tokens",
        "token",
        "program",
        "TOK_AUGMENT",
};

static int neoast_lexer_next_wrapper(void* matcher, void* dest, void* error_ctx)
{
    int tok = neoast_lexer_next((NeoastMatcher*)matcher, (NeoastValue*)dest, error_ctx);

    // EOF and INVALID
    if (tok <= 0) return tok;

    if (tok < NEOAST_ASCII_MAX)
    {
         int t_tok = (int32_t)neoast_ascii_mappings[tok];
         if (t_tok <= NEOAST_ASCII_MAX)
         {
             fprintf(stderr, "Lexer return '%c' (%d) which has not been explicitly defined as a token",
                     tok, tok);
             fflush(stderr);
             exit(1);
         }

         return t_tok - NEOAST_ASCII_MAX;
    }
    return tok - NEOAST_ASCII_MAX;
}

static GrammarParser parser = {
        .ascii_mappings = neoast_ascii_mappings,
        .grammar_rules = neoast_grammar_rules,
        .token_names = neoast_token_names,
        .destructors = neoast_token_destructors,
        .parser_error = parsing_error_cb,
        .parser_reduce = (parser_reduce) neoast_reduce_handler,
        .grammar_n = 36,
        .token_n = TOK_AUGMENT - NEOAST_ASCII_MAX,
        .action_token_n = 27
};

/********************************* PARSING TABLE *********************************/
static const
uint32_t cc_parsing_table[] = {
        0x00000000, 0x00000000, 0x00000000, 0x4000000E, 0x4000000C, 0x40000004, 0x4000000B, 0x40000001, 0x40000002, 0x40000009, 0x40000007, 0x00000000, 0x40000003, 0x40000006, 0x4000000A, 0x4000000F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000005, 0x4000000D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000008,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000014, 0x40000015, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000019, 0x40000015, 0x00000000,
        0x00000000, 0x8000001B, 0x00000000, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x00000000, 0x8000001B, 0x8000001B, 0x8000001B, 0x8000001B, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x4000001B, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000003F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000040, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x20000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000041, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x40000043, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000042, 0x40000015, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x40000048, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000047, 0x40000015, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000036, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000004, 0x00000000, 0x4000000E, 0x4000000C, 0x40000004, 0x4000000B, 0x40000001, 0x40000002, 0x40000009, 0x40000007, 0x00000000, 0x40000003, 0x40000006, 0x4000000A, 0x4000000F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000025, 0x4000000D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000027, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000010, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000011, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000012, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000013, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000014, 0x00000000, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x00000000, 0x80000014, 0x80000014, 0x80000014, 0x80000014, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000016, 0x00000000, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x00000000, 0x80000016, 0x80000016, 0x80000016, 0x80000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000022, 0x00000000, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x00000000, 0x80000022, 0x80000022, 0x80000022, 0x80000022, 0x40000017, 0x00000000, 0x80000022, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000022, 0x80000022, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000018, 0x40000015, 0x00000000,
        0x00000000, 0x80000020, 0x00000000, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x00000000, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x80000020, 0x00000000, 0x80000020, 0x80000020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000020, 0x80000020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000021, 0x00000000, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x00000000, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x80000021, 0x00000000, 0x80000021, 0x80000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000021, 0x80000021, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000023, 0x00000000, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x00000000, 0x80000023, 0x80000023, 0x80000023, 0x80000023, 0x00000000, 0x00000000, 0x80000023, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000023, 0x80000023, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000015, 0x00000000, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x00000000, 0x80000015, 0x80000015, 0x80000015, 0x80000015, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x8000001A, 0x00000000, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x00000000, 0x8000001A, 0x8000001A, 0x8000001A, 0x8000001A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001F, 0x00000000, 0x00000000, 0x4000001D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001C, 0x00000000, 0x4000001E, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x4000002B, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001F, 0x00000000, 0x00000000, 0x4000001D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000022, 0x40000023, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001F, 0x00000000, 0x00000000, 0x4000001D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000021, 0x00000000, 0x4000001E, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000006, 0x00000000, 0x00000000, 0x80000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000009, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000029, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000001F, 0x00000000, 0x00000000, 0x4000001D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000028, 0x40000023, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000037, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000013, 0x00000000, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x00000000, 0x80000013, 0x80000013, 0x80000013, 0x80000013, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000002A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000000B, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000007, 0x00000000, 0x00000000, 0x80000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000038, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x4000002C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000002F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000002E, 0x4000002D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x4000003E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x80000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000002F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000002E, 0x4000003D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000030, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x40000034, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000033, 0x40000031, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000035, 0x40000015, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000003C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000024, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000003A, 0x8000000C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000001E, 0x8000001E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000039, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000001F, 0x8000001F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000032, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000011, 0x00000000, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x00000000, 0x80000011, 0x80000011, 0x80000011, 0x80000011, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x8000000E, 0x00000000, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x00000000, 0x8000000E, 0x8000000E, 0x8000000E, 0x8000000E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000001D, 0x8000001D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x40000034, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000033, 0x4000003B, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000035, 0x40000015, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x8000000D, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x80000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x80000003, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x8000001C, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000019, 0x00000000, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x00000000, 0x80000019, 0x80000019, 0x80000019, 0x80000019, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000018, 0x00000000, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x00000000, 0x80000018, 0x80000018, 0x80000018, 0x80000018, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000017, 0x00000000, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x00000000, 0x80000017, 0x80000017, 0x80000017, 0x80000017, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x8000000F, 0x00000000, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x00000000, 0x8000000F, 0x8000000F, 0x8000000F, 0x8000000F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000044, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000045, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000046, 0x40000015, 0x00000000,
        0x00000000, 0x80000012, 0x00000000, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x00000000, 0x80000012, 0x80000012, 0x80000012, 0x80000012, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x80000010, 0x00000000, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x00000000, 0x80000010, 0x80000010, 0x80000010, 0x80000010, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000049, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x4000004A, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000017, 0x00000000, 0x00000000, 0x40000016, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000026, 0x40000015, 0x00000000,
};

/******************************* PARSER DEFINITIONS ******************************/
static int parser_initialized = 0;
uint32_t cc_init()
{
    if (!parser_initialized)
    {
        
        parser_initialized = 1;
    }

    return 0;
}

void cc_free()
{
    if (parser_initialized)
    {
        
        parser_initialized = 0;
    }
}

void* cc_allocate_buffers()
{
    return parser_allocate_buffers(
            1024,
            1024,
            sizeof(NeoastValue),
            offsetof(NeoastValue, position)
    );
}

void cc_free_buffers(void* self)
{
    parser_free_buffers((ParserBuffers*)self);
}

typeof(__cc__t_.file) cc_parse_len(void* error_ctx, void* buffers_, const char* input, uint32_t input_len)
{
    NeoastInput* lexer_input = input_new_from_buffer(input, input_len);

    typeof(__cc__t_.file) ret;
    ret = cc_parse_input(error_ctx, buffers_, lexer_input);

    input_free(lexer_input);
    return ret;
}

typeof(__cc__t_.file) cc_parse(void* error_ctx, void* buffers_, const char* input)
{
    return cc_parse_len(error_ctx, buffers_, input, strlen(input));
}

typeof(__cc__t_.file) cc_parse_input(void* error_ctx, void* buffers_, NeoastInput* input)
{
    ParserBuffers* buffers = (ParserBuffers*) buffers_;
    parser_reset_buffers(buffers);

    NeoastMatcher* ll_inst = matcher_new(input);
    NEOAST_STACK_PUSH(ll_inst->lexing_state, LEX_STATE_DEFAULT);

    int32_t output_idx = parser_parse_lr(
            &parser, error_ctx, cc_parsing_table,
            buffers, ll_inst, neoast_lexer_next_wrapper);

    matcher_free(ll_inst);

    if (output_idx < 0)
        return (typeof(__cc__t_.file))0;

    return ((NeoastValue*)buffers->value_table)[output_idx].value.file;
}

/************************************ BOTTOM *************************************/


static inline void ll_add_to_brace(const char* lex_text, uint32_t len)
{
    if (len + brace_buffer.n >= brace_buffer.s)
    {
        brace_buffer.s *= 2;
        brace_buffer.buffer = realloc(brace_buffer.buffer, brace_buffer.s);
    }

    strncpy(brace_buffer.buffer + brace_buffer.n, lex_text, len);
    brace_buffer.n += len;
}

static inline void ll_match_brace(const TokenPosition* start_position)
{
    brace_buffer.s = 1024;
    brace_buffer.buffer = malloc(brace_buffer.s);
    brace_buffer.n = 0;
    brace_buffer.start_position = *start_position;
    brace_buffer.counter = 1;
}


