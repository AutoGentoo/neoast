#include <string.h>
#include "cg_lexer.h"

void put_lexer_rule_action(
        const char* grammar_file_path,
        struct LexerRuleProto* self,
        const char* state_name,
        uint32_t regex_i,
        FILE* fp)
{
    fprintf(fp, "static int32_t\nll_rule_%s_%02d(const char* yytext, "
    CODEGEN_UNION"* yyval, "
                 "unsigned int len, "
                 "ParsingStack* lex_state, "
                 "TokenPosition* position"
                 ")\n{\n"
                 "    (void) yytext;\n"
                 "    (void) yyval;\n"
                 "    (void) len;\n"
                 "    (void) lex_state;\n"
                 "    (void) position;\n"
                 "    {", state_name, regex_i);

    // Put a line directive to tell the compiler
    // where to original code resides
    if (self->position.line && grammar_file_path)
    {
        fprintf(fp, "\n#line %d \"%s\"\n", self->position.line, grammar_file_path);
    }

    fputs(self->function, fp);
    fputs("}\n    return -1;\n}\n\n", fp);
}

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

uint32_t put_lexer_rule(LexerRule* self, const char* state_name, uint32_t offset, uint32_t rule_i, FILE* fp)
{
    if (self->expr)
    {
        fprintf(fp, "        {"
                    ".expr = (lexer_expr) ll_rule_%s_%02d, "
                    ".regex_raw = &ll_rules_state_%s_regex_table[%d],"
                    "}, // %s\n",
                state_name,  rule_i, state_name, offset, self->regex_raw);
    } else
    {
        // TODO Add support for quick token optimization in code gen
    }

    return strlen(self->regex_raw) + 1;
}

void put_lexer_state_rules(LexerRule* rules, uint32_t rules_n,
                           const char* state_name, FILE* fp)
{
    // First we need to build the regex table
    fprintf(fp, "static const char ll_rules_state_%s_regex_table[] = {\n", state_name);
    for (uint32_t i = 0; i < rules_n; i++)
    {
        put_lexer_rule_regex(&rules[i], fp);
    }
    fputs("};\n\n", fp);

    fprintf(fp, "static LexerRule ll_rules_state_%s[] = {\n", state_name);
    uint32_t offset = 0;
    for (int i = 0; i < rules_n; i++)
    {
        offset += put_lexer_rule(&rules[i], state_name, offset, i, fp);
    }

    fputs("};\n\n", fp);
}

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

void put_lexer_states(const char* const* states_names, int states, FILE* fp)
{
    fputs("static LexerRule* __neoast_lexer_rules[] = {\n", fp);
    for (int i = 0; i < states; i++)
    {
        fprintf(fp, "        ll_rules_state_%s,\n", states_names[i]);
    }

    fputs("};\n\n", fp);
}