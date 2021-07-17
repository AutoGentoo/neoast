#include "cg_lexer.h"

void put_lexer_rule_regex(std::ostream& os, const char* regex)
{
    os << "        ";
    for (const char* iter = regex; *iter; iter++)
    {
        os << variadic_string("0x%02x, ", *iter);
    }

    // Null terminator
    os << "0x00,";

    // String comment
    os << " // " << regex << "\n";
}

void put_lexer_rule_count(std::ostream &os, const std::vector<up<CGLexerState>>& states)
{
    os << "static const uint32_t lexer_rule_n[] = {";

    int is_first = true;
    for (const auto& i : states)
    {
        if (is_first)
        {
            os << i->get_rule_n();
            is_first = false;
        }
        else
        {
            os << ", " << i->get_rule_n();
        }
    }

    os << "};\n";
}

void put_lexer_states(std::ostream &os, const std::vector<up<CGLexerState>> &states)
{
    put_lexer_rule_count(os, states);

    os << "static LexerRule* __neoast_lexer_rules[] = {\n";
    for (const auto& i : states)
    {
        os << variadic_string("        ll_rules_state_%s,\n", i->get_name().c_str());
    }

    os << "};\n\n";
}

void CGLexerRule::put_action(std::ostream &os) const
{
    os << "static int32_t\n"
       << get_name()
       << "(const char* yytext, "
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
          "    {";
    code.put(os);
    os << "}\n    return -1;\n}\n\n";
}

uint32_t CGLexerRule::put_rule(std::ostream& os, uint32_t regex_i) const
{
    os << "        {.expr = (lexer_expr) " << get_name() << ", "
       << variadic_string(".regex = &ll_rules_state_%s_regex_table[%d]}, // %s\n",
                          parent->get_name().c_str(), regex_i, regex.c_str());

    return regex.size() + 1;
}

std::string CGLexerRule::get_name() const
{
    return variadic_string("ll_rule_%s_%02d", parent->get_name().c_str(), index);
}

CGLexerRule::CGLexerRule(const CGLexerState* parent, int index, MacroEngine &m_engine, LexerRuleProto* iter)
: code(iter->function, &iter->position),
  parent(parent), index(index), regex(m_engine.expand(iter->regex))
{
}

void CGLexerState::put(std::ostream &os) const
{
    for (const auto& rule : rules)
    {
        rule.put_action(os);
    }

    // First we need to build the regex table
    os << variadic_string("static const char ll_rules_state_%s_regex_table[] = {\n", name.c_str());
    for (const auto& i : rules)
    {
        put_lexer_rule_regex(os, i.regex.c_str());
    }
    os << "};\n\n";

    os << variadic_string("static LexerRule ll_rules_state_%s[] = {\n", name.c_str());
    uint32_t offset = 0;
    for (const auto& i : rules)
    {
        offset += i.put_rule(os, offset);
    }

    os << "};\n\n";
}
