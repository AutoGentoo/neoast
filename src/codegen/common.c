#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "codegen.h"

KeyVal* key_val_build(const TokenPosition* p, key_val_t type, char* key, char* value)
{
    KeyVal* self = malloc(sizeof(KeyVal));
    assert(p);
    self->position = *p;

    self->next = NULL;
    self->back = NULL;
    self->type = type;
    self->key = key;
    self->value = value;

    return self;
}

struct Token* build_token(const TokenPosition* position, char* name)
{
    struct Token* self = malloc(sizeof(struct Token));
    if (strncmp(name, "ASCII_CHAR_0x", 13) == 0)
    {
        emit_error(position, "Token name may not start with 'ASCII_CHAR_0x'");
        return NULL;
    }

    assert(position);
    self->position = *position;

    self->name = name;
    self->next = NULL;
    self->ascii = 0;
    return self;
}

char* get_ascii_token_name(char value)
{
    char* name = NULL;
    (void)asprintf(&name, "ASCII_CHAR_0x%02X", value);
    return name;
}

char get_ascii_from_name(const char* name)
{
    assert(strncmp(name, "ASCII_CHAR_0x", 13) == 0);
    return (char)strtoul(name + 13, NULL, 16);
}

struct Token* build_token_ascii(const TokenPosition* position, char value)
{
    struct Token* self = malloc(sizeof(struct Token));

    assert(position);
    self->position = *position;

    self->name = get_ascii_token_name(value);
    self->next = NULL;
    self->ascii = value;
    return self;
}

void tokens_free(struct Token* self)
{
    struct Token* next;
    while (self)
    {
        next = self->next;
        if (self->name) free(self->name);
        free(self);
        self = next;
    }
}

void key_val_free(KeyVal* self)
{
    KeyVal* next;
    if (self->back)
    {
        self = self->back;
    }

    while (self)
    {
        next = self->next;
        if (self->key) free(self->key);

        assert(self->value);
        free(self->value);

        free(self);
        self = next;
    }
}

void lexer_rule_free(struct LexerRuleProto* self)
{
    struct LexerRuleProto* next;
    while (self)
    {
        next = self->next;

        if (self->lexer_state)
        {
            assert(!self->function);
            assert(!self->regex);
            assert(self->state_rules);
            lexer_rule_free(self->state_rules);
            free(self->lexer_state);
        }
        else
        {
            assert(self->function);
            assert(self->regex);
            assert(!self->state_rules);
            free(self->function);
            free(self->regex);
        }

        free(self);
        self = next;
    }
}

void grammar_rule_single_free(struct GrammarRuleSingleProto* self)
{
    struct GrammarRuleSingleProto* next;
    while (self)
    {
        next = self->next;
        assert(self->function);
        free(self->function);
        tokens_free(self->tokens);
        free(self);
        self = next;
    }
}

void grammar_rule_multi_free(struct GrammarRuleProto* self)
{
    struct GrammarRuleProto* next;
    while (self)
    {
        next = self->next;
        assert(self->name);
        free(self->name);
        grammar_rule_single_free(self->rules);
        free(self);
        self = next;
    }
}

void file_free(struct File* self)
{
    key_val_free(self->header);
    lexer_rule_free(self->lexer_rules);
    grammar_rule_multi_free(self->grammar_rules);
    free(self);
}
