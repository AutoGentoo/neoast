//
// Created by tumbar on 1/16/21.
//

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
