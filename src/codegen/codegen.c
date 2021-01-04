//
// Created by tumbar on 1/4/21.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "stdio.h"
#include "codegen.h"

#define CODEGEN_AUGMENT_TOKEN "__TOK_AUGMENT__"
#define CODEGEN_UNION "NeoastValue"

#define CODEGEN_ERROR(...) \
do {                                \
    fprintf(stderr, "Error: ");     \
    fprintf(stderr, __VA_ARGS__);   \
    fprintf(stderr, "\n");          \
    return -1;                      \
} while(0)

typedef struct {
    const char* state_name;
    int state;
    int rule_count;
    struct LexerRuleProto** rules;
} LexerStateStructure;

static inline
void put_enum(int start, int n, const char* const* names, FILE* fp)
{
    if (!n)
    {
        fputs("enum {};\n\n", fp);
        return;
    }

    fputs("enum\n{\n", fp);
    fprintf(fp, "    %s = %d,\n", names[0], start++);
    for (int i = 1; i < n; i++)
    {
        fprintf(fp, "    %s, // %d\n", names[i], start++);
    }
    fputs("};\n\n", fp);
}

static inline
void put_lexer_rule_action(struct LexerRuleProto* self, FILE* fp)
{
    fprintf(fp, "static I32\nll_rule_%p(const char* yytext, "
                CODEGEN_UNION"* yyval, "
                "unsigned int len, "
                "Stack* lex_state)\n{\n"
                "    (void) lex_text;\n"
                "    (void) lex_val;\n"
                "    (void) len;\n"
                "    (void) lex_state;\n"
                "    {", self);
    fputs(self->function, fp);
    fputs("}\n    return -1;\n}\n\n", fp);
}

static inline
void put_lexer_rules(LexerStateStructure* self, FILE* fp)
{
    fprintf(fp, "static LexerRule ll_rules_state_%d[] = {\n", self->state);
    for (int i = 0; i < self->rule_count; i++)
    {
        if (self->rules[i]->function)
        {
            fprintf(fp, "        {.regex_raw = \"%s\", .expr = (lexer_expr) ll_rule_%p},\n",
                    self->rules[i]->regex, self->rules[i]);
        }
        else
        {
            // TODO Add support for quick token optimization in code gen
        }
    }

    fputs("};\n", fp);
}

static inline
int codegen_index(const char* const* array, const char* to_find, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (strcmp(array[i], to_find) == 0)
        {
            return i;
        }
    }

    return -1;
}

int codegen_write(const struct File* self, FILE* fp)
{
    // Codegen steps:
    //   1. Write the header
    //   2. Generate enums and union
    //   3. Dump lexer actions
    //   4. Dump parser actions
    //   5. Dump lexer rules structures (all states)
    //   6. Dump parser rules structure
    //   7. Generate table and write
    //   8. Generate entry point and buffer creation

    struct KeyVal* _header = NULL;
    struct KeyVal* _union = NULL;
    struct KeyVal* _start = NULL;

    int action_n = 0, token_n = 0,
        typed_token_n = 0, precedence_n = 0,
        macro_n = 0, lex_state_n = 0;

    // Iterate a single time though the header data
    // Count all of the different header option types
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_HEADER:
                if (_header)
                {
                    CODEGEN_ERROR("cannot have two %%top definitions");
                }

                _header = iter;
                break;
            case KEY_VAL_UNION:
                if (_union)
                {
                    CODEGEN_ERROR("cannot have two %%union definitions");
                }

                _union = iter;
                break;
            case KEY_VAL_START:
                if (_start)
                {
                    CODEGEN_ERROR("cannot have two %%start definitions");
                }

                _start = iter;
                break;
            case KEY_VAL_TOKEN:
                action_n++;
                token_n++;
                break;
            case KEY_VAL_TOKEN_TYPE:
                action_n++;
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_TYPE:
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_OPTION:
                break;
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                precedence_n++;
                break;
            case KEY_VAL_MACRO:
                macro_n++;
                break;
            case KEY_VAL_STATE:
                lex_state_n++;
                break;
        }
    }

    if (!_start || !_union)
    {
        CODEGEN_ERROR("%%start and %%union are required");
    }

    if (!action_n || token_n == action_n)
    {
        CODEGEN_ERROR("no lexer or parser tokens are defined");
    }

    // Now that we know the count of every type, we can initialize them
    int action_i = 0, grammar_i = action_n,
        typed_token_i = 0, lex_state_i = 0,
        macro_i = 0, precedence_i = 0;
    const char** tokens = malloc(sizeof(char*) * (token_n + 1)); // +1 for TOK_AUGMENT
    const struct KeyVal** typed_tokens = malloc(sizeof(struct KeyVal*) * typed_token_n);

    const char** lexer_states = malloc(sizeof(char*) * (lex_state_n));
    const struct KeyVal** macros = malloc(sizeof(struct KeyVal*) * macro_n);
    const struct KeyVal** precedences = malloc(sizeof(struct KeyVal*) * precedence_n);

    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_TOKEN:
                tokens[action_i++] = iter->key;
                printf("%d => %s %s\n", action_i - 1, iter->key, tokens[action_i - 1]);
                break;
            case KEY_VAL_TOKEN_TYPE:
                tokens[action_i++] = iter->key;
                printf("%d => %s %s\n", action_i - 1, iter->key, tokens[action_i - 1]);
                typed_tokens[typed_token_i++] = iter;
                break;
            case KEY_VAL_TYPE:
                tokens[grammar_i++] = iter->key;
                typed_tokens[typed_token_i++] = iter;
                break;
            case KEY_VAL_STATE:
                lexer_states[lex_state_i++] = iter->key;
                break;
            case KEY_VAL_MACRO:
                macros[macro_i++] = iter;
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                precedences[precedence_i++] = iter;
                break;
            default:
                break;
        }
    }

    tokens[grammar_i++] = CODEGEN_AUGMENT_TOKEN;

    assert(typed_token_i == typed_token_n);
    assert(action_i == action_n);
    assert(grammar_i - 1 == token_n);
    assert(precedence_i == precedence_n);
    assert(lex_state_i == lex_state_n);
    assert(macro_i == macro_n);

    // Write the header information
    if (_header)
    {
        fprintf(fp, "%s\n", _header->value);
    }

    fprintf(fp, "typedef union {%s} " CODEGEN_UNION ";\n\n", _union->value);
    fputs("// Tokens\n", fp);
    put_enum(1, token_n, tokens, fp);

    fputs("// Lexer states\n", fp);
    put_enum(1, lex_state_n, lexer_states, fp);

    // Count the number of lexer rules in each state
    // Also dump all lexer rule actions
    int* rule_count = calloc(lex_state_n + 1, sizeof(int));
    for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        int state_id;
        if (iter->lexer_state)
        {
            state_id = codegen_index(lexer_states, iter->lexer_state, lex_state_n);

            if (state_id == -1)
            {
                CODEGEN_ERROR("invalid state name '%s'", iter->lexer_state);
            }
        }
        else
        {
            state_id = 0;
        }

        rule_count[state_id]++;

        put_lexer_rule_action(iter, fp);
    }

    LexerStateStructure* lex_states_structs = malloc(sizeof(LexerStateStructure) * (lex_state_n + 1));
    for (int i = 0; i < lex_state_n + 1; i++)
    {
        LexerStateStructure* current = &lex_states_structs[i];
        if (i)
            current->state_name = lexer_states[i - 1];
        else
            current->state_name = "LEX_STATE_DEFAULT";

        current->state = i;
        current->rule_count = 0;

        // Add all of the lexer rules with this state
        // to this array
        current->rules = malloc(sizeof(struct LexerRuleProto*) * rule_count[i]);

        for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
        {
            if (iter->lexer_state && i != 0)
                continue;
            if (iter->lexer_state && strcmp(iter->lexer_state, current->state_name) != 0)
                continue;

            // iter is in this state
            current->rules[current->rule_count++] = iter;
        }

        assert(current->rule_count == rule_count[i]);

        put_lexer_rules(current, fp);
    }

    return 0;
}