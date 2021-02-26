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


#ifndef NEOAST_REGEX_H
#define NEOAST_REGEX_H
#include <cre2.h>

typedef struct MacroEngine_prv MacroEngine;
typedef struct Macro_prv Macro;

struct Macro_prv
{
    char* name;
    char* expanded_regex;

    Macro* next;
};

struct MacroEngine_prv
{
    Macro* head;
    cre2_regexp_t* macro_pattern;
    cre2_options_t* opts;
};

MacroEngine* macro_engine_init();
void macro_engine_free(MacroEngine* self);
void macro_engine_register(MacroEngine* self, const char* name, const char* regex);
char* regex_expand(MacroEngine* self, const char* regex);
int regex_verify(MacroEngine* self, const char* regex);

#endif //NEOAST_REGEX_H
