//
// Created by tumbar on 1/4/21.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <lexer.h>
#include "stdio.h"
#include "codegen.h"
#include "regex.h"
#include <parsergen/canonical_collection.h>
#include <util/util.h>
#include <stddef.h>

#define CODEGEN_UNION "NeoastValue"

#define CODEGEN_ERROR(...) \
do {                                \
    fprintf(stderr, "Error: ");     \
    fprintf(stderr, __VA_ARGS__);   \
    fprintf(stderr, "\n");          \
    return -1;                      \
} while(0)

struct Options {
    // Should we dump the table
    int debug_table;
    int disable_locks;
    char* debug_ids;
    char* prefix;
    parser_t parser_type; // LALR(1) or CLR(1)

    unsigned long max_lex_tokens;
    unsigned long max_token_len;
    unsigned long max_lex_state_depth;
};

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
        fprintf(fp, "    %s, // %d 0x%03X", names[i], start, start);
        if (strncmp(names[i], "ASCII_CHAR_0x", 13) == 0)
        {
            uint8_t c = strtoul(&names[i][13], NULL, 16);
            fprintf(fp, " '%c'\n", c);
        }
        else
        {
            fputc('\n', fp);
        }
        start++;
    }
    fputs("};\n\n", fp);
}

static inline
void put_lexer_rule_action(struct LexerRuleProto* self, FILE* fp)
{
    fprintf(fp, "static int32_t\nll_rule_%p(const char* yytext, "
                CODEGEN_UNION"* yyval, "
                "unsigned int len, "
                "ParsingStack* lex_state)\n{\n"
                "    (void) yytext;\n"
                "    (void) yyval;\n"
                "    (void) len;\n"
                "    (void) lex_state;\n"
                "    {", self);
    fputs(self->function, fp);
    fputs("}\n    return -1;\n}\n\n", fp);
}

static inline
const char* check_grammar_arg_skip(const char* start, const char* search)
{
    // Assume we are not in comment or string
    int current_red_zone = -1;
    static const struct
    {
        const char* start_str;
        const char* end_str;
        uint32_t len[2];
    } red_zone_desc[] = {
            {"//", "\n", {2, 1}},
            {"/*", "*/", {2, 2}},
            {"\"", "\"", {1, 1}}
    };

    for (; (start < search || current_red_zone != -1) && *start; start++)
    {
        if (*start == '\\') // escape next character
            continue;

        for (int i = 0; i < sizeof(red_zone_desc) / sizeof(red_zone_desc[0]); i++)
        {
            if (current_red_zone == -1)
            {
                // Check for the start of a red zone
                if (strncmp(start, red_zone_desc[i].start_str, red_zone_desc[i].len[0]) == 0)
                {
                    current_red_zone = i;
                    start += red_zone_desc[i].len[0] - 1;
                    break;
                }
            } else
            {
                // Check for the end of a red zone
                if (strncmp(start, red_zone_desc[i].end_str, red_zone_desc[i].len[1]) == 0)
                {
                    current_red_zone = -1;
                    start += red_zone_desc[i].len[1] - 1;
                    break;
                }
            }
        }
    }

    return start;
}

static inline
int get_named_token(const char* token_name, const char* const* tokens, uint32_t token_n)
{
    for (int i = 0; i < token_n; i++)
    {
        if (strcmp(tokens[i], token_name) == 0)
            return i;
    }

    fprintf(stderr, "Invalid token name '%s'\n", token_name);
    return -1;
}

static inline
const char* put_grammar_rule_arg(
        const char* search,
        struct GrammarRuleProto* parent,
        struct GrammarRuleSingleProto* self,
        const char** tokens,
        const struct KeyVal** typed_tokens,
        uint32_t token_n,
        FILE* fp)
{
    if (search[1] == '$')
    {
        int expression_token = get_named_token(parent->name, tokens, token_n);
        if (expression_token == -1)
        {
            return NULL;
        }

        assert(typed_tokens[expression_token]);

        fprintf(fp, "dest->%s", typed_tokens[expression_token]->value);
        return search + 2;
    }

    char* out;
    uint32_t arg_num = strtoul(search + 1, &out, 10);
    if (arg_num == 0)
    {
        fprintf(stderr, "Invalid argument index '0', use '$$' for destination\n");
        return NULL;
    }

    struct Token* tok = self->tokens;
    int i;
    for (i = 0; i < arg_num - 1 && tok; i++)
    {
        tok = tok->next;
    }

    if (!tok)
    {
        fprintf(stderr, "Invalid argument index '%d', expression only has %d arguments\n", arg_num, i);
        return NULL;
    }

    int token_id = get_named_token(tok->name, tokens, token_n);
    if (!typed_tokens[token_id])
    {
        fprintf(stderr, "Token '%s' does not have a type\n", tok->name);
        return NULL;
    }

    fprintf(fp, "args[%d].%s", arg_num - 1, typed_tokens[token_id]->value);

    return out;
}

static inline
void put_grammar_rule_action(
        struct GrammarRuleProto* parent,
        struct GrammarRuleSingleProto* self,
        const char** tokens,
        const struct KeyVal** typed_tokens,
        uint32_t token_n,
        FILE* fp)
{
    fprintf(fp, "static void\ngg_rule_%p("
                CODEGEN_UNION"* dest, "
                CODEGEN_UNION"* args)\n{\n"
                "    (void) dest;\n"
                "    (void) args;\n"
                "    {", self->function);

    // Find all usages of $N or $$
    // Check if this usage should be ignored (comment or string)
    // Replace this usage with the appropriate C-value

    const char* search = NULL;
    const char* start = self->function;
    while ((search = strchr(start, '$')))
    {
        if ((search[1] >= '0' && search[1] <= '9') || search[1] == '$')
        {
            // Check if this string is in a comment or string
            const char* finish_red = check_grammar_arg_skip(start, search);
            if (finish_red > search)
            {
                // Skip this '$'
                search = finish_red;
            } else
            {
                // Print the content until this point
                fwrite(start, 1, search - start, fp);

                // Print the argument
                start = put_grammar_rule_arg(search, parent, self, tokens, typed_tokens, token_n, fp);
                if (!start)
                    return;
                continue;
            }
        }

        // Not an argument
        fwrite(start, 1, search - start, fp);
        start = search;
    }

    fputs(start, fp);
    fputs("}\n}\n\n", fp);
}

static inline
void put_lexer_rule_regex(LexerRule* self, FILE* fp)
{
    fputs("        ", fp);
    for (const char* iter = self->regex_raw; *iter; iter++)
    {
        fprintf(fp, "0x%02x, ", *iter);
    }

    // Null terminator
    fprintf(fp, "0x%02x,\n", 0);
}

static inline
uint32_t put_lexer_rule(LexerRule* self, const char* state_name, uint32_t offset, FILE* fp)
{
    if (self->expr)
    {
        fprintf(fp, "        {.regex_raw = &ll_rules_state_%s_regex_table[%d], .expr = (lexer_expr) ll_rule_%p}, // %s\n",
                state_name, offset, self->expr, self->regex_raw);
    } else
    {
        // TODO Add support for quick token optimization in code gen
    }

    return strlen(self->regex_raw) + 1;
}

static inline
void put_lexer_state_rules(LexerRule* rules, int rules_n, const char* state_name, FILE* fp)
{
    // First we need to build the regex table
    fprintf(fp, "static const char ll_rules_state_%s_regex_table[] = {\n", state_name);
    for (int i = 0; i < rules_n; i++)
    {
        put_lexer_rule_regex(&rules[i], fp);
    }
    fputs("};\n\n", fp);

    fprintf(fp, "static LexerRule ll_rules_state_%s[] = {\n", state_name);
    uint32_t offset = 0;
    for (int i = 0; i < rules_n; i++)
    {
        offset += put_lexer_rule(&rules[i], state_name, offset, fp);
    }

    fputs("};\n\n", fp);
}

static inline
void put_lexer_rule_count(const uint32_t* ll_rule_count, int lex_state_n, FILE* fp)
{
    fputs("static const uint32_t lexer_rule_n[] = {", fp);
    for (int i = 0; i < lex_state_n; i++)
    {
        if (i + 1 >= lex_state_n)
        {
            fprintf(fp, "%d", ll_rule_count[i]);
        }
        else
        {
            fprintf(fp, "%d, ", ll_rule_count[i]);
        }
    }
    fputs("};\n", fp);
}

static inline
void put_lexer_states(const char* const* states_names, int states, FILE* fp)
{
    fputs("static LexerRule* lexer_rules[] = {\n", fp);
    for (int i = 0; i < states; i++)
    {
        fprintf(fp, "        ll_rules_state_%s,\n", states_names[i]);
    }

    fputs("};\n\n", fp);
}

static inline
void put_grammar_table_and_rules(
        struct GrammarRuleProto* rule_symbols,
        GrammarRule* rules,
        const char* const* tokens,
        uint32_t rule_n,
        FILE* fp)
{
    // First put the grammar table
    fputs("static const\nunsigned int grammar_token_table[] = {\n", fp);
    uint32_t grammar_i = 0;
    for (struct GrammarRuleProto* rule_iter = rule_symbols;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        fprintf(fp, "        /* %s */\n", rule_iter->name);
        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            if (grammar_i == 0)
            {
                grammar_i++;
                fputs("        /* ACCEPT */ ", fp);
            }
            else
            {
                fprintf(fp,"        /* R%02d */ ", grammar_i++);
            }
            if (rule_single_iter->tokens)
            {
                for (struct Token* tok = rule_single_iter->tokens; tok; tok = tok->next)
                {
                    fprintf(fp, "%s,%c", tok->name, tok->next ? ' ' : '\n');
                }
            } else
            {
                fputs("// empty rule\n", fp);
            }
        }

        fputc('\n', fp);
    }
    fputs("};\n\n", fp);

    // Print the grammar rules
    uint32_t grammar_offset_i = 0;
    fputs("static const\nGrammarRule grammar_rules[] = {\n", fp);
    for (int i = 0; i < rule_n; i++)
    {
        if (rules[i].expr)
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d], .expr=(parser_expr) gg_rule_%p},\n",
                    tokens[rules[i].token - ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i,
                    rules[i].expr);
        }
        else
        {
            fprintf(fp, "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d]},\n",
                    tokens[rules[i].token - ASCII_MAX],
                    rules[i].tok_n,
                    grammar_offset_i);
        }

        grammar_offset_i += rules[i].tok_n;
    }
    fputs("};\n\n", fp);
}

static inline
void put_parsing_table(const uint32_t* parsing_table, CanonicalCollection* cc, FILE* fp)
{
    int i = 0;
    fputs("static const\nuint32_t GEN_parsing_table[] = {\n", fp);
    for (int state_i = 0; state_i < cc->state_n; state_i++)
    {
        fputs("        ", fp);
        for (int tok_i = 0; tok_i < cc->parser->token_n; tok_i++, i++)
        {
            fprintf(fp,"0x%08X,%c", parsing_table[i], tok_i + 1 >= cc->parser->token_n ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
}

static inline
void put_ascii_mappings(const uint32_t ascii_mappings[ASCII_MAX], FILE* fp)
{
    fputs("static const\nuint32_t ascii_mappings[ASCII_MAX] = {\n", fp);
    int i = 0;
    for (int row = 0; i < ASCII_MAX; row++)
    {
        fputs("        ", fp);
        for (int col = 0; col < 6 && i < ASCII_MAX; col++, i++)
        {
            fprintf(fp, "0x%03X,%c", ascii_mappings[i], (col + 1 >= 6) ? '\n' : ' ');
        }
    }
    fputs("};\n\n", fp);
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

static int codegen_parse_bool(const char* bool_str)
{
    if (strcmp(bool_str, "TRUE") == 0
        || strcmp(bool_str, "true") == 0
        || strcmp(bool_str, "True") == 0
        || strcmp(bool_str, "1") == 0)
        return 1;
    else if (strcmp(bool_str, "FALSE") == 0
             || strcmp(bool_str, "false") == 0
             || strcmp(bool_str, "False") == 0
             || strcmp(bool_str, "0") == 0)
        return 0;

    fprintf(stderr, "Unable to parse boolean value '%s', assuming FALSE\n", bool_str);
    return 0;
}

static void codegen_handle_option(
        struct Options* self,
        const struct KeyVal* option)
{
    if (strcmp(option->key, "parser_type") == 0)
    {
        if (strcmp(option->value, "LALR(1)") == 0)
        {
            self->parser_type = LALR_1;
        }
        else if (strcmp(option->value, "CLR(1)") == 0)
        {
            self->parser_type = CLR_1;
        }
        else
        {
            fprintf(stderr, "Invalid parser type '%s', supported types: 'LALR(1)', 'CLR(1)'\n", option->value);
        }
    }
    else if (strcmp(option->key, "debug_table") == 0)
    {
        self->debug_table = codegen_parse_bool(option->value);
    }
    else if (strcmp(option->key, "disable_locks") == 0)
    {
        self->disable_locks = codegen_parse_bool(option->value);
    }
    else if (strcmp(option->key, "debug_ids") == 0)
    {
        self->debug_ids = option->value;
    }
    else if (strcmp(option->key, "prefix") == 0)
    {
        self->prefix = option->value;
    }
    else if (strcmp(option->key, "max_lex_tokens") == 0)
    {
        self->max_lex_tokens = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "max_token_len") == 0)
    {
        self->max_token_len = strtoul(option->value, NULL, 0);
    }
    else if (strcmp(option->key, "max_lex_state_depth") == 0)
    {
        self->max_lex_state_depth = strtoul(option->value, NULL, 0);
    }
    else
    {
        fprintf(stderr, "Unsupported option '%s', ignoring\n", option->key);
    }
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
    struct KeyVal* _bottom = NULL;
    struct KeyVal* _union = NULL;
    struct KeyVal* _start = NULL;

    int action_n = 1, token_n = 1, // reserved eof
        typed_token_n = 0, precedence_n = 0,
        lex_state_n = 1;

    struct Options options = {
            .debug_table = 0,
            .disable_locks = 0,
            .parser_type = LALR_1,
            .debug_ids = NULL,
            .prefix = "neoast",
            .max_token_len = 1024,
            .max_lex_tokens = 1024,
            .max_lex_state_depth = 16
    };

    MacroEngine* m_engine = macro_engine_init();

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
            case KEY_VAL_BOTTOM:
                if (_bottom)
                {
                    CODEGEN_ERROR("cannot have two %%bottom definitions");
                }

                _bottom = iter;
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
            case KEY_VAL_TOKEN_ASCII:
                action_n++;
                token_n++;
                break;
            case KEY_VAL_TOKEN_TYPE:
            case KEY_VAL_TOKEN_TYPE_ASCII:
                action_n++;
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_TYPE:
                token_n++;
                typed_token_n++;
                break;
            case KEY_VAL_OPTION:
                codegen_handle_option(&options, iter);
                break;
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                precedence_n++;
                break;
            case KEY_VAL_MACRO:
                macro_engine_register(m_engine, iter->key, iter->value);
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

    token_n++; // augment token

    // Now that we know the count of every type, we can initialize them
    int action_i = 0, grammar_i = action_n,
            typed_token_i = 0, lex_state_i = 1,
            macro_i = 0;
    const char** tokens = calloc(token_n, sizeof(char*));
    const struct KeyVal** typed_tokens = calloc(token_n, sizeof(struct KeyVal*));

    const char** lexer_states = malloc(sizeof(char*) * (lex_state_n));
    uint8_t* precedence_table = calloc(token_n, sizeof(uint8_t));

    uint32_t ascii_mappings[ASCII_MAX] = {0};

    tokens[action_i++] = "EOF";
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        switch (iter->type)
        {
            case KEY_VAL_TOKEN:
                tokens[action_i++] = iter->key;
                break;
            case KEY_VAL_TOKEN_TYPE:
                typed_tokens[action_i] = iter;
                tokens[action_i++] = iter->key;
                typed_token_i++;
                break;
            case KEY_VAL_TOKEN_ASCII:
                ascii_mappings[iter->options] = action_i + ASCII_MAX;
                tokens[action_i++] = iter->key;
                break;
            case KEY_VAL_TOKEN_TYPE_ASCII:
                ascii_mappings[iter->options] = action_i + ASCII_MAX;
                typed_tokens[action_i] = iter;
                tokens[action_i++] = iter->key;
                typed_token_i++;
                break;
            case KEY_VAL_TYPE:
                typed_tokens[grammar_i] = iter;
                tokens[grammar_i++] = iter->key;
                typed_token_i++;
                break;
            case KEY_VAL_STATE:
                lexer_states[lex_state_i++] = iter->key;
                break;
            default:
                break;
        }
    }

    // Generate the precedence table
    for (struct KeyVal* iter = self->header; iter; iter = iter->next)
    {
        int token_id;
        switch (iter->type)
        {
            case KEY_VAL_LEFT:
            case KEY_VAL_RIGHT:
                token_id = codegen_index(tokens, iter->key, token_n);
                if (token_id == -1 || token_id >= action_n)
                    fprintf(stderr, "Invalid token for precedence '%s'\n", iter->key);

                precedence_table[token_id] = iter->type == KEY_VAL_LEFT ? PRECEDENCE_LEFT : PRECEDENCE_RIGHT;

                break;
            default:
                break;
        }
    }

    tokens[grammar_i++] = "TOK_AUGMENT";

    assert(typed_token_i == typed_token_n);
    assert(action_i == action_n);
    assert(grammar_i == token_n);
    assert(lex_state_i == lex_state_n);

    // Write the header information
    fputs("#define NEOAST_PARSER_CODEGEN___C\n"
          "#include <neoast.h>\n"
          "#include <string.h>\n", fp);

    if (_header)
    {
        fputs(_header->value, fp);
        fputc('\n', fp);
    }

    fprintf(fp, "typedef union {%s} " CODEGEN_UNION ";\n\n", _union->value);
    fputs("// Tokens\n", fp);
    put_enum(ASCII_MAX + 1, token_n - 1, tokens + 1, fp);

    // Map a lexer state id to each state
    lexer_states[0] = "LEX_STATE_DEFAULT";
    for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        if (iter->lexer_state && codegen_index(lexer_states, iter->lexer_state, lex_state_n) == -1)
        {
            lexer_states[lex_state_n++] = iter->lexer_state;
        }
    }

    fputs("// Lexer states\n", fp);
    put_enum(0, lex_state_n, lexer_states, fp);

    // Dump all lexer rule actions
    // Count the number of lexer rules in each state
    uint32_t* ll_rule_count = calloc(lex_state_n, sizeof(uint32_t));
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
        } else
        {
            state_id = 0;
        }

        ll_rule_count[state_id]++;

        put_lexer_rule_action(iter, fp);
    }

    LexerRule** ll_rules = malloc(sizeof(LexerRule*) * lex_state_n);
    for (int i = 0; i < lex_state_n; i++)
    {
        LexerRule* current = malloc(sizeof(LexerRule) * ll_rule_count[i]);
        ll_rules[i] = current;
        int iterator = 0;

        // We're going to build a large array with every string embedded
        uint32_t regex_length = 0;
        for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
        {
            if (iter->lexer_state && i != 0)
                continue;
            if (iter->lexer_state && strcmp(iter->lexer_state, lexer_states[i]) != 0)
                continue;

            LexerRule* ll_rule = &current[iterator++];
            ll_rule->expr = (lexer_expr) iter;
            ll_rule->regex_raw = regex_expand(m_engine, iter->regex);
            regex_length += strlen(ll_rule->regex_raw) + 1; // We need to null terminator
            if (!regex_verify(m_engine, ll_rule->regex_raw))
            {
                CODEGEN_ERROR("Failed to compile regular expression '%s'\n", ll_rule->regex_raw);
            }

            ll_rule->tok = 0;
        }

        assert(iterator == ll_rule_count[i]);
        put_lexer_state_rules(current, ll_rule_count[i], lexer_states[i], fp);
    }

    put_lexer_rule_count(ll_rule_count, lex_state_n, fp);
    put_lexer_states(lexer_states, lex_state_n, fp);

    uint32_t grammar_n = 0;
    uint32_t grammar_tok_n = 0;
    for (struct GrammarRuleProto* rule_iter = self->grammar_rules;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            put_grammar_rule_action(rule_iter, rule_single_iter, tokens, typed_tokens, token_n, fp);
            grammar_n++;

            // Find the number of tokens in this rule
            for (struct Token* tok_iter = rule_single_iter->tokens; tok_iter; tok_iter = tok_iter->next)
            {
                grammar_tok_n++;
            }
        }
    }

    // Add the augment rule
    struct Token augment_rule_tok = {
            .next = NULL,
            .name = _start->key
    };
    struct GrammarRuleSingleProto augment_rule = {
            .next = NULL,
            .tokens = &augment_rule_tok
    };
    struct GrammarRuleProto augment_rule_parent = {
            .next = self->grammar_rules,
            .rules = &augment_rule,
            .name = "TOK_AUGMENT"
    };

    struct GrammarRuleProto* all_rules = &augment_rule_parent;
    grammar_n++;
    grammar_tok_n++;

    GrammarRule* gg_rules = malloc(sizeof(GrammarRule) * grammar_n);
    int32_t* grammar_table = malloc(sizeof(uint32_t) * grammar_tok_n);
    uint32_t grammar_tok_offset_c = 0;
    grammar_i = 0;

    for (struct GrammarRuleProto* rule_iter = all_rules;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        int rule_tok = codegen_index(tokens, rule_iter->name, token_n);
        if (rule_tok == -1)
        {
            fprintf(stderr, "Could not find token for rule '%s'\n", rule_iter->name);
            exit(1);
        }

        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            uint32_t grammar_table_offset = grammar_tok_offset_c;
            uint32_t gg_tok_n = 0;
            // Add the tokens to the token tables
            for (struct Token* tok_iter = rule_single_iter->tokens; tok_iter; tok_iter = tok_iter->next)
            {
                grammar_table[grammar_tok_offset_c] = codegen_index(tokens, tok_iter->name, token_n) + ASCII_MAX;
                assert(grammar_table[grammar_tok_offset_c] != -1 && "Invalid token name");
                grammar_tok_offset_c++;
                gg_tok_n++;
            }

            // Construct the rule
            GrammarRule* gg_current = &gg_rules[grammar_i++];
            gg_current->expr = (parser_expr) rule_single_iter->function;
            gg_current->token = rule_tok + ASCII_MAX;
            gg_current->grammar = (uint32_t*) &grammar_table[grammar_table_offset];
            gg_current->tok_n = gg_tok_n;
        }
    }

    assert(grammar_tok_offset_c == grammar_tok_n);
    put_grammar_table_and_rules(all_rules, gg_rules, tokens, grammar_n, fp);

    // Initialize the parser
    GrammarParser parser = {
            .token_n = token_n - 1,
            .token_names = tokens,
            .lex_state_n = lex_state_n,
            .action_token_n = action_n,
            .lex_n = ll_rule_count,
            .lexer_rules = ll_rules,
            .grammar_n = grammar_n,
            .grammar_rules = gg_rules,
            .ascii_mappings = ascii_mappings
    };

    // Dump parser instantiation
    fputs("static const\nchar* token_names[] = {\n", fp);
    for (int i = 0; i < token_n - 1; i++)
    {
        fprintf(fp, "        \"%s\",\n", tokens[i]);
    }
    fputs("};\n\n", fp);

    put_ascii_mappings(ascii_mappings, fp);

    fprintf(fp, "static GrammarParser parser = {\n"
                "        .token_n = TOK_AUGMENT - ASCII_MAX,\n"
                "        .token_names = token_names,\n"
                "        .lex_state_n = %d,\n"
                "        .action_token_n = %d,\n"
                "        .lex_n = lexer_rule_n,\n"
                "        .lexer_rules = lexer_rules,\n"
                "        .grammar_n = %d,\n"
                "        .grammar_rules = grammar_rules,\n"
                "        .ascii_mappings = ascii_mappings\n",
                lex_state_n, action_n, grammar_n
    );
    fputs("};\n\n", fp);

    CanonicalCollection* cc = canonical_collection_init(&parser);
    canonical_collection_resolve(cc, LALR_1);

    uint32_t* parsing_table = canonical_collection_generate(cc, precedence_table);

    if (options.debug_table)
    {
        fprintf(fp, "// Token names:\n");
        char* fallback = malloc(token_n + 1);
        for (int i = 0; i < token_n - 1; i++)
        {
            if (i == 0)
            {
                fallback[i] = '$';
            }
            else if (i < action_n)
            {
                fallback[i] = (char)('a' + (char)i - 1);
            }
            else
            {
                fallback[i] = (char)('A' + (char)i - (action_n));
            }
        }

        if (!options.debug_ids) options.debug_ids = fallback;
        for (int i = 0; i < token_n - 1; i++)
        {
            fprintf(fp, "//  %s => '%c'\n", tokens[i], options.debug_ids[i]);
        }

        fputs("\n", fp);
        dump_table(parsing_table, cc, options.debug_ids, 0, fp, "//  ");
        free(fallback);
    }

    // Dump the actual parsing table
    put_parsing_table(parsing_table, cc, fp);

    // Dump the exported symbols
    fprintf(fp, "static int parser_initialized = 0;\n"
                "uint32_t %s_init()\n{\n"
                "    if (!parser_initialized)\n"
                "    {\n"
                "        uint32_t error = parser_init(&parser);\n"
                "        if (error)\n"
                "            return error;\n\n"
                "        parser_initialized = 1;\n"
                "    }\n"
                "    return 0;\n"
                "}\n\n",
                options.prefix);
    fprintf(fp, "void %s_free()\n"
                "{\n"
                "    if (parser_initialized)\n"
                "    {\n"
                "         parser_free(&parser);\n"
                "         parser_initialized = 0;\n"
                "    }\n"
                "}\n\n",
                options.prefix);

    fprintf(fp, "void* %s_allocate_buffers()\n"
                "{\n"
                "    return parser_allocate_buffers(%lu, %lu, %lu, sizeof("CODEGEN_UNION"));\n"
                "}\n\n",
                options.prefix,
                options.max_lex_tokens,
                options.max_token_len,
                options.max_lex_state_depth);

    fprintf(fp, "void %s_free_buffers(void* self)\n"
                "{\n"
                "    parser_free_buffers(self);\n"
                "}\n\n",
                options.prefix);

    fprintf(fp, "static " CODEGEN_UNION " t;\n"
                "typeof(t.%s) %s_parse(void* buffers, const char* input)\n"
                "{\n"
                "    parser_reset_buffers((ParserBuffers*)buffers);\n"
                "    \n"
                "    uint64_t input_len = strlen(input);\n"
                "    if (lexer_fill_table(input, input_len, &parser, (ParserBuffers*)buffers) == -1) return (typeof(t.%s))0;\n"
                "    int32_t output_idx = parser_parse_lr(&parser, GEN_parsing_table, (ParserBuffers*)buffers);\n"
                "    \n"
                "    if (output_idx < 0) return (typeof(t.%s))0;\n"
                "    return ((" CODEGEN_UNION "*)((ParserBuffers*)buffers)->value_table)[output_idx].%s;\n"
                "}\n\n",
                _start->value, options.prefix,
                _start->value, _start->value, _start->value);

    if (_bottom)
    {
        fputs(_bottom->value, fp);
        fputc('\n', fp);
    }

    canonical_collection_free(cc);
    free(parsing_table);
    free(gg_rules);
    free(grammar_table);
    free(typed_tokens);
    free(lexer_states);
    free(precedence_table);
    free(tokens);

    for (int i = 0; i < lex_state_n; i++)
    {
        free((char*)ll_rules[i]->regex_raw);
        free(ll_rules[i]);
    }
    free(ll_rules);
    free(ll_rule_count);
    macro_engine_free(m_engine);

    return 0;
}