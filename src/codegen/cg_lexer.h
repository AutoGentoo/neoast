#ifndef NEOAST_CG_LEXER_H
#define NEOAST_CG_LEXER_H

#include <codegen/codegen.h>
#include <codegen/codegen_priv.h>
#include <reflex/pattern.h>


struct CGLexerState;
struct CGLexerRule
{
    Code code;
    const CGLexerState* parent;
    int index;
    std::string regex;

    explicit CGLexerRule(const CGLexerState* parent,
                         int index,
                         MacroEngine& m_engine,
                         LexerRuleProto* iter);

    std::string get_name() const;

    void put_action(std::ostream& os) const;
    uint32_t put_rule(std::ostream& os, uint32_t regex_i) const;
};

class CGLexerState
{
    int id;
    std::string name;
    std::vector<CGLexerRule> rules;

public:
    CGLexerState(int id, std::string name) : id(id), name(std::move(name)) {}
    void put(std::ostream& os) const;
    void add_rule(MacroEngine& m_engine, LexerRuleProto* iter)
    {
        rules.emplace_back(this, rules.size(), m_engine, iter);
    }

    const std::string& get_name() const { return name; }
    size_t get_rule_n() const { return rules.size(); }
};

void put_lexer_states(std::ostream &os, const std::vector<CGLexerState> &states);

#endif //NEOAST_CG_LEXER_H
