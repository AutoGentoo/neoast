#ifndef NEOAST_CG_LEXER_H
#define NEOAST_CG_LEXER_H

#include "codegen.h"

void put_lexer_rule_action(
        const char* grammar_file_path,
        struct LexerRuleProto* self,
        const char* state_name,
        uint32_t regex_i,
        FILE* fp);

void put_lexer_rule_regex(LexerRule* self, FILE* fp);
uint32_t put_lexer_rule(LexerRule* self, const char* state_name, uint32_t offset, uint32_t rule_i, FILE* fp);
void put_lexer_state_rules(LexerRule* rules, uint32_t rules_n,
                           const char* state_name, FILE* fp);

void put_lexer_rule_count(const uint32_t* ll_rule_count, int lex_state_n, FILE* fp);
void put_lexer_states(const char* const* states_names, int states, FILE* fp);

#endif //NEOAST_CG_LEXER_H
