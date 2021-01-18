//
// Created by tumbar on 1/16/21.
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "regex.h"

MacroEngine* macro_engine_init()
{
    MacroEngine* self = malloc(sizeof(MacroEngine));
    self->head = NULL;
    self->opts = cre2_opt_new();
    cre2_opt_set_one_line(self->opts, 1);
    assert(self->opts && "re2 failed to allocate memory for options");

    const char* pattern = "\\{[A-z][A-z0-9_]*\\}";

    self->macro_pattern = cre2_new(pattern, (int)strlen(pattern), self->opts);
    assert(self->macro_pattern && "re2 failed to allocate memory for regex pattern");
    assert(!cre2_error_code(self->macro_pattern) && "Failed to build macro");

    return self;
}

static void macro_free_prv(Macro* self)
{
    Macro* next;
    while (self)
    {
        next = self->next;
        free(self->name);
        free(self->expanded_regex);
        free(self);
        self = next;
    }
}

void macro_engine_free(MacroEngine* self)
{
    macro_free_prv(self->head);
    cre2_opt_delete(self->opts);
    cre2_delete(self->macro_pattern);
    free(self);
}

const char* macro_engine_get_macro(MacroEngine* self, const char* name, int name_length)
{
    for (Macro* iter = self->head; iter; iter = iter->next)
    {
        if (strncmp(iter->name, name, name_length) == 0 && !iter->name[name_length])
        {
            return iter->expanded_regex;
        }
    }

    return NULL;
}

char* regex_expand(MacroEngine* self, const char* regex)
{
    int length = (int)strlen(regex);
    cre2_string_t input_str = {
            .data = regex,
            .length = length
    };

    cre2_string_t match_str;
    char* expanded_pattern = NULL;

    uint32_t offset = 0;

    while (cre2_match(self->macro_pattern, regex, length, offset,
                      length, CRE2_UNANCHORED, &match_str, 1))
    {
        // Find the macro's pattern
        uint32_t old_offset = offset;
        offset = (match_str.data - regex) + match_str.length;
        const char* macro_pattern = macro_engine_get_macro(self, match_str.data + 1, match_str.length - 2);
        if (macro_pattern == NULL)
        {
            // Pattern not found, don't try to expand this
            continue;
        }

        if (expanded_pattern)
        {
            char* old_pattern = expanded_pattern;
            expanded_pattern = NULL;
            asprintf(&expanded_pattern, "%s%.*s%s",
                     old_pattern,
                     (int)(match_str.data - regex) - old_offset, regex + old_offset,
                     macro_pattern);
            free(old_pattern);
        }
        else
        {
            asprintf(&expanded_pattern, "%.*s%s",
                    (int)(match_str.data - regex) - old_offset, regex + old_offset,
                    macro_pattern);
        }
    }

    // Add the rest of the non-matched regex string
    if (expanded_pattern)
    {
        char* old_pattern = expanded_pattern;
        expanded_pattern = NULL;
        asprintf(&expanded_pattern, "%s%s", old_pattern, input_str.data + offset);
        free(old_pattern);
        return expanded_pattern;
    }

    // We didn't expand anything, simply dup the input
    return strdup(regex);
}

void macro_engine_register(MacroEngine* self, const char* name, const char* regex)
{
    Macro* item = malloc(sizeof(Macro));

    // Recursively expand this macro
    item->expanded_regex = regex_expand(self, regex);
    item->name = strdup(name);

    item->next = self->head;
    self->head = item;
}

int regex_verify(MacroEngine* self, const char* regex)
{
    // Check that this pattern compiles
    cre2_regexp_t* rex = cre2_new(regex, (int)strlen(regex), self->opts);
    if (!regex)
    {
        fprintf(stderr, "Failed to allocate memory for regex '%s'\n", regex);
        return 0;
    }

    if (cre2_error_code(rex))
    {
        cre2_delete(rex);
        return 0;
    }

    cre2_delete(rex);
    return 1;
}
