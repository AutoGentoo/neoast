#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "compiler.h"

kv* declare_option(const TokenPosition* p, char* name, char* value)
{
    (void) declare_option;

    return key_val_build(p, KEY_VAL_OPTION, name, value);
}

kv* declare_start(const TokenPosition* p, char* union_type, char* rule)
{
    (void) declare_start;

    return key_val_build(p, KEY_VAL_START, union_type, rule);
}

static
kv* tokens_to_kv(
        struct Token* tokens,
        const char* key,
        key_val_t type,
        key_val_t ascii_type,
        int allow_ascii)
{
    kv* next = NULL;
    kv* first = NULL;
    for (struct Token* iter = tokens; iter; iter = iter->next)
    {
        if (iter->ascii && !allow_ascii)
        {
            emit_error(&iter->position, "ASCII tokens not allowed here");
            break;
        }

        kv* curr = key_val_build(&iter->position,
                                 iter->ascii ? ascii_type : type,
                                 key ? strdup(key) : NULL,
                                 iter->name);
        iter->name = NULL;

        curr->next = next;
        next = curr;

        if (!first)
        {
            first = curr;
        }
    }

    tokens_free(tokens);
    assert(first->next == NULL);
    first->back = next;
    return first;
}

kv* declare_tokens(struct Token* tokens)
{
    (void) declare_tokens;

    /* Build a list of key_val */
    /* Order is not important so we can build backs */
    return tokens_to_kv(tokens, NULL, KEY_VAL_TOKEN, KEY_VAL_TOKEN_ASCII, 1);
}

kv* declare_typed_tokens(char* type, struct Token* tokens)
{
    (void) declare_typed_tokens;

    kv* out = tokens_to_kv(tokens, type, KEY_VAL_TOKEN_TYPE, KEY_VAL_TOKEN_TYPE, 0);
    free(type);
    return out;
}

kv* declare_types(char* type, struct Token* tokens)
{
    (void) declare_types;

    kv* out = tokens_to_kv(tokens, type, KEY_VAL_TYPE, KEY_VAL_TYPE, 0);
    if (type) free(type);
    return out;
}

kv* declare_destructor(const TokenPosition* p, char* type, char* action)
{
    (void) declare_destructor;

    return key_val_build(p, KEY_VAL_DESTRUCTOR, type, action);
}

kv* declare_right(struct Token* tokens)
{
    (void) declare_right;

    return tokens_to_kv(tokens, NULL, KEY_VAL_RIGHT, KEY_VAL_RIGHT, 1);
}

kv* declare_left(struct Token* tokens)
{
    (void) declare_left;

    return tokens_to_kv(tokens, NULL, KEY_VAL_LEFT, KEY_VAL_LEFT, 1);
}

kv* declare_top(const TokenPosition* p, char* action)
{
    (void) declare_top;

    return key_val_build(p, KEY_VAL_TOP, NULL, action);
}

kv* declare_include(const TokenPosition* p, char* action)
{
    (void) declare_include;

    return key_val_build(p, KEY_VAL_INCLUDE, NULL, action);
}

kv* declare_bottom(const TokenPosition* p, char* action)
{
    (void) declare_bottom;

    return key_val_build(p, KEY_VAL_BOTTOM, NULL, action);
}

kv* declare_union(const TokenPosition* p, char* action)
{
    (void) declare_union;

    return key_val_build(p, KEY_VAL_UNION, NULL, action);
}

lr_p* declare_state_rule(const TokenPosition* p, char* state_name, lr_p* rules)
{
    (void) declare_state_rule;

    lr_p* out = malloc(sizeof(struct LexerRuleProto));
    assert(p);
    out->position = *p;
    out->lexer_state = state_name;
    out->state_rules = rules;
    out->regex = NULL;
    out->function = NULL;
    out->next = NULL;
    return out;
}

lr_p* declare_lexer_rule(const TokenPosition* p, char* regex, char* action)
{
    (void) declare_lexer_rule;

    lr_p* out = malloc(sizeof(struct LexerRuleProto));
    assert(p);
    out->position = *p;
    out->lexer_state = NULL;
    out->state_rules = NULL;
    out->regex = regex;
    out->function = action;
    out->next = NULL;
    return out;
}

grs_p* declare_single_grammar(const TokenPosition* p, struct Token* tokens, char* action)
{
    (void) declare_single_grammar;

    grs_p* out = malloc(sizeof(struct GrammarRuleSingleProto));
    assert(p);
    out->position = *p;
    out->next = NULL;
    out->tokens = tokens;
    out->function = action;
    return out;
}

gr_p* declare_grammar(const TokenPosition* p, char* name, grs_p* grammars)
{
    (void) declare_grammar;

    gr_p* out = malloc(sizeof(struct GrammarRuleProto));
    assert(p);
    out->position = *p;
    out->next = NULL;
    out->name = name;
    out->rules = grammars;
    return out;
}

f_p* declare_file(kv* header, lr_p* lexer_rules, gr_p* grammars)
{
    (void) declare_file;

    f_p* out = malloc(sizeof(struct File));
    out->header = header;
    out->lexer_rules = lexer_rules;
    out->grammar_rules = grammars;
    return out;
}
