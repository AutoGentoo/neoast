#ifndef NEOAST_COMPILER_H
#define NEOAST_COMPILER_H

#include "codegen.h"

typedef KeyVal kv;
typedef struct LexerRuleProto lr_p;
typedef struct GrammarRuleSingleProto grs_p;
typedef struct GrammarRuleProto gr_p;
typedef struct File f_p;

kv* declare_option(const TokenPosition* p, char* name, char* value);
kv* declare_start(const TokenPosition* p, char* union_type, char* rule);
kv* declare_tokens(struct Token* tokens);
kv* declare_typed_tokens(char* type, struct Token* tokens);
kv* declare_types(char* type, struct Token* tokens);
kv* declare_destructor(const TokenPosition* p, char* type, char* action);
kv* declare_right(struct Token* tokens);
kv* declare_left(struct Token* tokens);
kv* declare_bottom(const TokenPosition* p, char* action);
kv* declare_top(const TokenPosition* p, char* action);
kv* declare_union(const TokenPosition* p, char* action);

lr_p* declare_lexer_rule(const TokenPosition* p, char* regex, char* action);
lr_p* declare_state_rule(const TokenPosition* p, char* state_name, lr_p* rules);
grs_p* declare_single_grammar(const TokenPosition* p, struct Token* tokens, char* action);

gr_p* declare_grammar(const TokenPosition* p, char* name, grs_p* grammars);
f_p* declare_file(kv* header, lr_p* lexer_rules, gr_p* grammars);

#endif //NEOAST_COMPILER_H
