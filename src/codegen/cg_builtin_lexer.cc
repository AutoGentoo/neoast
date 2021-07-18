#include "cg_builtin_lexer.h"
#include <reflex/pattern.h>

struct CGBuiltinLexerState;
struct CGBuiltinLexerRule
{
    Code code;
    const CGBuiltinLexerState* parent;
    int index;
    std::string regex;

    explicit CGBuiltinLexerRule(const CGBuiltinLexerState* parent,
                                int index,
                                MacroEngine& m_engine,
                                LexerRuleProto* iter);

    std::string get_name() const;

    void put_action(std::ostream& os) const;
    uint32_t put_rule(std::ostream& os, uint32_t regex_i) const;
};

class CGBuiltinLexerState
{
    int id;
    std::string name;
    std::vector<CGBuiltinLexerRule> rules;

public:
    CGBuiltinLexerState(int id_, std::string name) : id(id_), name(std::move(name)) {}
    void put(std::ostream& os) const;
    void add_rule(MacroEngine& m_engine, LexerRuleProto* iter)
    {
        rules.emplace_back(this, rules.size(), m_engine, iter);
    }

    const std::string& get_name() const { return name; }
    size_t get_rule_n() const { return rules.size(); }
};

void put_lexer_rule_regex(std::ostream& os, const char* regex)
{
    os << "        ";
    for (const char* iter = regex; *iter; iter++)
    {
        os << variadic_string("0x%02x, ", *iter);
    }

    os << "0x00,"  // null terminator
       << " // " << regex << "\n"; // string comment
}

void put_lexer_rule_count(std::ostream &os, const std::vector<up<CGBuiltinLexerState>>& states)
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

void CGBuiltinLexerRule::put_action(std::ostream &os) const
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
          "    {"
       << code.get_simple()
       << "}\n    return -1;\n}\n\n";
}

uint32_t CGBuiltinLexerRule::put_rule(std::ostream& os, uint32_t regex_i) const
{
    os << "        {.expr = (lexer_expr) " << get_name() << ", "
       << variadic_string(".regex = &ll_rules_state_%s_regex_table[%d]}, // %s\n",
                          parent->get_name().c_str(), regex_i, regex.c_str());

    return regex.size() + 1;
}

std::string CGBuiltinLexerRule::get_name() const
{
    return variadic_string("ll_rule_%s_%02d", parent->get_name().c_str(), index);
}

CGBuiltinLexerRule::CGBuiltinLexerRule(const CGBuiltinLexerState* parent, int index, MacroEngine &m_engine, LexerRuleProto* iter)
: code(iter->function, &iter->position),
  parent(parent), index(index), regex(m_engine.expand(iter->regex))
{
}

void CGBuiltinLexerState::put(std::ostream &os) const
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

struct CGBuiltinLexerImpl
{
    std::vector<up<CGBuiltinLexerState>> states;
    const Options &options;

    explicit CGBuiltinLexerImpl(const Options &options_)
    : options(options_)
    {}
};

CGBuiltinLexer::~CGBuiltinLexer()
{
    delete impl_;
}

CGBuiltinLexer::CGBuiltinLexer(const File* self, MacroEngine &m_engine_, const Options &options)
: impl_(new CGBuiltinLexerImpl(options))
{
    int state_n = 0;
    impl_->states.emplace_back(new CGBuiltinLexerState(state_n++, "LEX_STATE_DEFAULT"));

    for (struct LexerRuleProto* iter = self->lexer_rules; iter; iter = iter->next)
    {
        if (iter->lexer_state)
        {
            // Make sure the name does not overlap
            bool duplicate_state = false;
            for (const auto &iter_state : impl_->states)
            {
                if (iter_state->get_name() == iter->lexer_state)
                {
                    emit_error(&iter->position,
                               "Multiple definition of lexer state '%s'",
                               iter->lexer_state);
                    duplicate_state = true;
                    break;
                }
            }

            if (duplicate_state)
            {
                continue;
            }

            impl_->states.emplace_back(new CGBuiltinLexerState(state_n++, iter->lexer_state));
            auto &ls = impl_->states[impl_->states.size() - 1];

            for (struct LexerRuleProto* iter_s = iter->state_rules; iter_s; iter_s = iter_s->next)
            {
                assert(iter_s->regex);
                assert(iter_s->function);
                assert(!iter_s->lexer_state);
                assert(!iter_s->state_rules);

                ls->add_rule(m_engine_, iter_s);
            }
        }
        else
        {
            // Add to the default lexing state
            impl_->states[0]->add_rule(m_engine_, iter);
        }
    }
}

void CGBuiltinLexer::put_header(std::ostream &os) const
{
    os << "#include <codegen/builtin_lexer/builtin_lexer.h>\n"
       << "#include <stddef.h> // offsetof\n"
       << "#ifdef __NEOAST_GET_STATES__\n";
    std::vector<std::string> state_names;
    for (const auto& i : impl_->states) state_names.push_back(i->get_name());
    put_enum(0, state_names, os);
    os << "#endif // __NEOAST_GET_STATES__\n\n";
}

void CGBuiltinLexer::put_global(std::ostream &os) const
{
    for (const auto& i : impl_->states)
    {
        i->put(os);
    }

    put_lexer_rule_count(os, impl_->states);

    os << "static const LexerRule* builtin_neoast_lexer_rules[] = {\n";
    for (const auto& i : impl_->states)
    {
        os << variadic_string("        ll_rules_state_%s,\n", i->get_name().c_str());
    }

    os << "};\n\n";
    os << "static void* builtin_lexer_parent = NULL;\n";
}

std::string CGBuiltinLexer::get_init() const
{
    return "builtin_lexer_parent = builtin_lexer_new("
           "builtin_neoast_lexer_rules, lexer_rule_n, "
           "sizeof(lexer_rule_n) / sizeof(lexer_rule_n[0]), "
           + (impl_->options.lexing_error_cb.empty() ? "nullptr"
              : impl_->options.lexing_error_cb) + ", offsetof(" + CODEGEN_STRUCT +
              ", position), __neoast_ascii_mappings);";
}

std::string CGBuiltinLexer::get_delete() const
{
    return "builtin_lexer_free(builtin_lexer_parent); "
           "builtin_lexer_parent = NULL;";
}

std::string CGBuiltinLexer::get_new_inst(const std::string &name) const
{
    return "void* " + name + " = builtin_lexer_instance_new(builtin_lexer_parent, input, input_len)";
}

std::string CGBuiltinLexer::get_del_inst(const std::string &name) const
{
    return "builtin_lexer_instance_free(" + name + ");";
}

std::string CGBuiltinLexer::get_ll_next(const std::string &name) const
{
    (void) name;
    return "builtin_lexer_next";
}
