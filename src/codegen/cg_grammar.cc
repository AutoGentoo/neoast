//
// Created by tumbar on 7/17/21.
//

#include "cg_grammar.h"


struct CGGrammar
{
    std::vector<uint32_t> &table;

    const GrammarRuleSingleProto* parent;
    sp<CGGrammarToken> return_type;
    std::vector<std::string> argument_types;

    Code action;

    uint32_t table_offset;
    uint32_t token_n;

    CGGrammar(std::vector<uint32_t> &table_,
              const CodeGen* cg,
              sp<CGGrammarToken> return_type_,
              const GrammarRuleSingleProto* self) :
        table(table_),
        action(self->function ? self->function : "", &self->position),
        return_type(std::move(return_type_)), token_n(0),
        parent(self)
    {
        assert(return_type.get());
        table_offset = table.size();
        argument_types.push_back(return_type->type);

        for (const auto* iter = self->tokens; iter; iter = iter->next)
        {
            const auto t = cg->get_token(iter->name);
            if (!t)
            {
                emit_error(&iter->position, "Undeclared token '%s'", iter->name);
                continue;
            }

            table.push_back(t->id + NEOAST_ASCII_MAX);
            token_n++;

            const auto* t_casted = dynamic_cast<CGTyped*>(t.get());
            if (t_casted)
            {
                argument_types.push_back(t_casted->type);
            }
            else
            {
                argument_types.emplace_back("");
            }
        }
    }

    GrammarRule initialize_grammar() const
    {
        GrammarRule gg;
        gg.expr = action.empty() ? nullptr : reinterpret_cast<parser_expr>(0x2);
        gg.token = return_type->id + NEOAST_ASCII_MAX;
        gg.grammar = reinterpret_cast<const uint32_t*>(&table[table_offset]);
        gg.tok_n = token_n;
        return gg;
    }

    void put_action(std::ostream& os, const Options& options, int i) const
    {
        if (action.empty())
        {
            return;
        }

        os << "static void\n"
           << variadic_string("gg_rule_r%02d(%s* dest, %s* args)\n{\n",
                              i, CODEGEN_STRUCT, CODEGEN_STRUCT)
           << "    (void) dest;\n"
           << "    (void) args;\n"
           << action.get_complex(options, argument_types, "dest", "args")
           << "}\n\n";
    }

    void put_grammar_entry(std::ostream& os) const
    {
        for (struct Token* tok = parent->tokens; tok; tok = tok->next)
        {
            os << variadic_string("%s,%c", tok->name, tok->next ? ' ' : '\n');
        }

        if (!parent->tokens)
        {
            os << "// empty rule\n";
        }
    }
};

struct CGGrammarsImpl
{
    const CodeGen* cg;
    std::vector<uint32_t> table;

    std::map<std::string, std::vector<CGGrammar>> rules_cg;
    std::vector<GrammarRule> rules;
    uint32_t grammar_n = 0;

    Token* augment_rule_tok;
    GrammarRuleSingleProto* augment_rule_single_tok;

    explicit CGGrammarsImpl(const CodeGen* cg_)
    : cg(cg_), augment_rule_tok(new Token()),
      augment_rule_single_tok(new GrammarRuleSingleProto())
    {
        // Initial augment rule
        augment_rule_tok->name = const_cast<char*>(cg->get_start_token());
        augment_rule_tok->position = NO_POSITION;
        augment_rule_tok->ascii = 0;
        augment_rule_tok->next = nullptr;

        augment_rule_single_tok->tokens = augment_rule_tok;
        augment_rule_single_tok->position = NO_POSITION;
        augment_rule_single_tok->function = nullptr;
        augment_rule_single_tok->next = nullptr;
    }

    ~CGGrammarsImpl()
    {
        delete augment_rule_tok;
        delete augment_rule_single_tok;
    }
};

CGGrammars::CGGrammars(const CodeGen* cg, const File* self)
: impl_(new CGGrammarsImpl(cg))
{
    // Add the augment rule
    {
        std::vector<CGGrammar> augment_grammar;
        auto tok_augment_return = std::static_pointer_cast<CGGrammarToken>(cg->get_token("TOK_AUGMENT"));
        if (!tok_augment_return)
        {
            emit_error(nullptr, "Expected augment rule to be a grammar token");
            return;
        }

        augment_grammar.emplace_back(impl_->table, cg, tok_augment_return,
                                     impl_->augment_rule_single_tok);
        impl_->rules_cg.emplace("TOK_AUGMENT", std::move(augment_grammar));
    }

    for (struct GrammarRuleProto* rule_iter = self->grammar_rules;
         rule_iter;
         rule_iter = rule_iter->next)
    {
        sp<CGToken> tok = cg->get_token(rule_iter->name);
        if (!tok)
        {
            emit_error(&rule_iter->position, "Could not find token for rule '%s'\n", rule_iter->name);
            continue;
        }
        else if (dynamic_cast<CGAction*>(tok.get()))
        {
            emit_error(&rule_iter->position, "Only %%type can be grammar rules");
            continue;
        }
        else if (impl_->rules_cg.find(rule_iter->name) != impl_->rules_cg.end())
        {
            emit_error(&rule_iter->position, "Redefinition of '%s' grammar", rule_iter->name);
            continue;
        }

        auto grammar = std::static_pointer_cast<CGGrammarToken>(tok);
        assert(grammar);

        std::vector<CGGrammar> grammars;
        for (struct GrammarRuleSingleProto* rule_single_iter = rule_iter->rules;
             rule_single_iter;
             rule_single_iter = rule_single_iter->next)
        {
            grammars.emplace_back(impl_->table, cg, grammar, rule_single_iter);
            impl_->grammar_n++;
        }

        impl_->rules_cg.emplace(rule_iter->name, grammars);
    }

    impl_->rules.reserve(impl_->grammar_n);
    for (const auto &rules : impl_->rules_cg)
    {
        for (const auto& rule : rules.second)
        {
            impl_->rules.emplace_back(rule.initialize_grammar());
        }
    }
}

CGGrammars::~CGGrammars()
{
    delete impl_;
}

void CGGrammars::put_table(std::ostream &os) const
{
    // Parsing table

    // First put the grammar table
    os << "static const\n"
          "unsigned int grammar_token_table[] = {\n";

    uint32_t gg_i = 0;
    for (const auto &rules : impl_->rules_cg)
    {
        os << variadic_string("        /* %s */\n", rules.first.c_str());
        for (const auto &rule : rules.second)
        {
            if (gg_i == 0)
            {
                gg_i++;
                os << "        /* ACCEPT */ ";
            }
            else
            {
                os << variadic_string("        /* R%02d */ ", gg_i++);
            }

            rule.put_grammar_entry(os);
        }

        os << "\n";
    }
    os << "};\n\n";
}

void CGGrammars::put_rules(std::ostream &os) const
{
    // Print the grammar rules
    uint32_t grammar_offset_i = 0;
    os << "static const\n"
          "GrammarRule __neoast_grammar_rules[] = {\n";

    uint32_t gg_i = 1;
    for (const auto &rule : impl_->rules)
    {
        if (rule.expr)
        {
            os << variadic_string(
                    "        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d], .expr=(parser_expr) gg_rule_r%02d},\n",
                    impl_->cg->get_tokens()[rule.token - NEOAST_ASCII_MAX].c_str(),
                    rule.tok_n,
                    grammar_offset_i,
                    gg_i++);
        }
        else
        {
            os << variadic_string("        {.token=%s, .tok_n=%d, .grammar=&grammar_token_table[%d]},\n",
                                  impl_->cg->get_tokens()[rule.token - NEOAST_ASCII_MAX].c_str(),
                                  rule.tok_n,
                                  grammar_offset_i);
        }

        grammar_offset_i += rule.tok_n;
    }
    os << "};\n\n";
}

void CGGrammars::put_actions(std::ostream &os) const
{
    int gg_i = 0;
    for (const auto &rules : impl_->rules_cg)
    {
        for (const auto &rule : rules.second)
        { rule.put_action(os, impl_->cg->get_options(), gg_i++); }
    }
}

uint32_t CGGrammars::size() const
{
    return impl_->grammar_n;
}

GrammarRule* CGGrammars::get() const
{
    return impl_->rules.data();
}
