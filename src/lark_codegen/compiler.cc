#include "compiler.h"

namespace neoast
{
    LRProduction::LRProduction(
            Compiler& c,
            BaseGrammar* return_token_,
            std::list<LRItem*> items,
            const ast::Token* action_,
            const ast::Token* alias_)
    : return_token(return_token_), production(std::move(items)),
    action(action_), alias(alias_)
    {
        // Resolve the type if not known yet
        if (!return_token->type)
        {
            if (production.empty())
            {
                c.context.emit_error(
                        &return_token->position,
                        "Generated production cannot be empty. Unable to deduce type");
                return;
            }

            // Get the GCD type
            return_token->type = production.front()->type;

            for (const auto& item : production)
            {
                return_token->type = return_token->type->common_base(item->type);
            }

            if (!return_token->type)
            {
                // No common base, must be Ast
                return_token->type = c.types.at(c.options["ast_class"]).get();
            }
        }
    }

    ProductionTree Compiler::resolve_expr(const ast::Expr* expr)
    {
        // The atom can be a variety of types
        auto* atom = dynamic_cast<const ast::Atom*>(expr);
        auto* expansion = dynamic_cast<const ast::Expansion*>(expr);
        auto* op_expr = dynamic_cast<const ast::OpExpr*>(expr);
        auto* or_expr = dynamic_cast<const ast::OrExpr*>(expr);

        ProductionTree out;

        if (expansion)
        {
            for (const auto& iter : expansion->all)
            {
                out.push(resolve_expr(iter.get()));
            }
        }
        else if (or_expr)
        {
            out.branch(
                    resolve_expr(or_expr->left.get()),
                    resolve_expr(or_expr->right.get()));
        }
        else if (op_expr)
        {
            TokenPosition position = get_position_from_expr(op_expr->inner.get());
            ProductionTree tree = resolve_expr(op_expr->inner.get());

#if 0
            // Check if there are manually defined bounds
            // Simplify them if needed
            if (op_expr->n1 >= 0)
            {
                if (op_expr->n2 >= 0 && op_expr->n1 != op_expr->n2)
                {
                    if (op_expr->n2 < op_expr->n1)
                    {
                        context.emit_error(
                                &position,
                                "{n1, n2}: n2 cannot be large than n1 in range");
                        return out;
                    }

                    // This expression is bounded on both ends
                    // Meaning it can be a range of repetitions
                    if (op_expr->n1 == 0 && op_expr->n2 == 1)
                    {
                        context.emit_warning(
                                &position,
                                "This range can be simplified to [ expr ]");
                        op = ast::ZERO_OR_ONE;
                    }
                    else
                    {
                        // Repeat a number of times between two bounds
                        generated_vectors.emplace_back(
                                std::move(ast::Token(&position, item->name)),
                                item->type);
                        Type* new_type = &generated_vectors.back();
                        BaseGrammar* inter_g = &generate_grammar(position, new_type);
                        std::list<LRItem*> repeat;

                        // Add the minimum
                        for (int i = 0; i < op_expr->n1; i++)
                        {
                            repeat.emplace_back(item);
                        }

                        // Build up each possibility
                        for (int j = op_expr->n1; j < op_expr->n2; j++)
                        {
                            repeat.emplace_back(item);
                            productions.emplace_back(*this, inter_g, repeat);
                        }
                        item = inter_g;
                    }
                }
                else
                {
                    if (op_expr->n2 == op_expr->n1)
                    {
                        context.emit_warning(
                                &position,
                                "Single range of %d used",
                                op_expr->n1);
                    }

                    if (op_expr->n1 == 0)
                    {
                        context.emit_warning(
                                &position,
                                "Zero repetitions, ignoring");
                        return out;
                    }
                    else if (op_expr->n1 == 1)
                    {
                        context.emit_warning(
                                &position,
                                "Redundant repetition specifier");
                    }
                    else
                    {
                        // Repeat an exact number of times
                        generated_vectors.emplace_back(
                                std::move(ast::Token(&position, item->name)),
                                item->type);
                        Type* new_type = &generated_vectors.back();
                        BaseGrammar* inter_g = &generate_grammar(position, new_type);
                        std::list<LRItem*> repeat;
                        for (int i = 0; i < op_expr->n1; i++)
                        {
                            repeat.emplace_back(item);
                        }

                        productions.emplace_back(*this, inter_g, repeat);
                        item = inter_g;
                    }
                }
            }
#endif

            switch (op_expr->op)
            {
                case ast::Op_NONE:
                    return tree;
                case ast::ZERO_OR_ONE:
                    // Build two productions:
                    //  no item, single item
                {
                    // Create a grammar to hold this expansion
                    // We use the same type here because all type's default value should be the
                    // empty or uninitialized case
                    ProductionTree inter_g = resolve_expr(op_expr->inner.get());
                    out.branch(inter_g, ProductionTree());
                }
                    break;
                case ast::ONE_OR_MORE:
                case ast::ZERO_OR_MORE:
                    // Build three or two productions:
                    //  (no item only if ZERO_OR_MORE), single item, recursive production
                {
                    Type* type_t = get_expr_type(op_expr, *this);
                    if (!type_t)
                    {
                        context.emit_error(&position, "Vectored type must simple token or grammar or "
                                                      "'or expression with common base class");
                        break;
                    }

                    auto* vector_type = dynamic_cast<AstVector*>(type_t);
                    assert(vector_type != nullptr);
                    Type* item_type = vector_type->element_type();

                    // Zero or more means a vector of the element
                    // Generate a new type to wrap this

                    // Create a grammar to hold this expansion
                    BaseGrammar* vector_grammar = &generate_grammar(position, vector_type);
                    BaseGrammar* item_grammar = &generate_grammar(position, item_type);

                    // Resolve each element as a grammar token
                    ProductionTree item_tree = resolve_expr(op_expr->inner.get());
                    for (auto& iter : item_tree)
                    {
                        productions.emplace_back(*this, item_grammar, iter);
                    }

                    // Build the vector grammar
                    if (op_expr->op == ast::ZERO_OR_MORE)
                    {
                        productions.emplace_back(*this, vector_grammar, std::initializer_list<LRItem*>{});
                    }

                    productions.emplace_back(*this, vector_grammar, std::initializer_list<LRItem*>{item_grammar});
                    productions.emplace_back(*this, vector_grammar, std::initializer_list<LRItem*>{item_grammar, vector_grammar});

                    // Actually add the vector to the grammar
                    out.push(vector_grammar);
                }
                    break;
            }
        }
        else if (atom)
        {
            // Search for the definition in either raw tokens or grammars
            auto token_it = tokens.find(atom->value->token->value);
            if (token_it != tokens.end())
            {
                out.push(&token_it->second);
            }
            else
            {
                // Check if it's an explicitly declared grammar
                auto grammar_it = grammars.find(atom->value->token->value);
                if (grammar_it != grammars.end())
                {
                    out.push(&grammar_it->second);
                }
                else
                {
                    context.emit_error(
                            atom->value->token.get(),
                            "Undeclared token or grammar");
                    return out;
                }
            }
        }

        return out;
    }

    ProductionTree Compiler::resolve_expr(const std::list<up<ast::Expr>> &exprs)
    {
        ProductionTree out;
        for (const auto& iter : exprs)
        {
            out.push(resolve_expr(iter.get()));
        }
        return out;
    }

    GrammarRule LRProduction::get_rule() const
    {
        if (!grammar_expanded)
        {
            auto* ptr = const_cast<LRProduction*>(this);
            ptr->grammar_expanded = up<tok_t[]>(new tok_t[production.size()]);
            int i = 0;
            for (const auto& iter : production)
            {
                ptr->grammar_expanded[i++] = iter->id;
            }
        }

        return {static_cast<tok_t>(return_token->id),
                static_cast<tok_t>(production.size()),
                grammar_expanded.get()};
    }

    void Compiler::do_s5()
    {
        for (auto& g_iter : grammars)
        {
            // Go through every AST production
            // And generate true LR productions
            auto grammar_it = grammars.find(g_iter.second.decl.name->value);
            if (grammar_it == grammars.end())
            {
                context.emit_error(g_iter.second.decl.name.get(),
                                   "Grammar not defined");
                continue;
            }

            for (auto& prod : g_iter.second.decl.productions)
            {
                auto tree = resolve_expr(prod->rule);
                bool is_empty = true;
                for (const auto& branch : tree)
                {
                    is_empty = false;
                    productions.emplace_back(
                            *this, &grammar_it->second,
                            branch,
                            prod->action.get(),
                            prod->alias.get());
                }

                if (is_empty)
                {
                    TokenPosition pos = get_position_from_expr(prod->rule);
                    context.emit_error(&pos, "Expression produced no productions");
                }
            }
        }
    }

    void Compiler::do_s6()
    {
        int curr_tok; // skip over 0, (EOF)
        curr_tok = NEOAST_ASCII_MAX + 1;

        // Label the terminals
        for (auto& tok : tokens)
        {
            tok.second.id = curr_tok++;
        }

        // Label the grammar rules
        for (auto& grm : grammars)
        {
            grm.second.id = curr_tok++;
        }
        for (auto& grm : generated_grammars)
        {
            grm.second.id = curr_tok++;
        }
    }

    void Compiler::do_s7()
    {
        if (!s1.start)
        {
            context.emit_error(nullptr, "%%start rule must be defined");
            return;
        }

        auto start_it = grammars.find(s1.start->value);
        if (start_it == grammars.end())
        {
            context.emit_error(s1.start, "Grammar rule not defined");
            return;
        }

        std::vector<GrammarRule> grammar_rules;
        std::vector<const TokenPosition*> rule_positions;

        // Add the augment rule
        grammar_rules.push_back({
            static_cast<tok_t>(start_it->second.id),
            1,
            reinterpret_cast<tok_t*>(&start_it->second.id)
        });
        rule_positions.push_back(nullptr);

        for (const auto& prod : productions)
        {
            grammar_rules.push_back(prod.get_rule());
            if (prod.action)
                rule_positions.push_back(prod.action);
            else if (prod.alias)
                rule_positions.push_back(prod.alias);
            else
                rule_positions.push_back(&prod.return_token->position);
        }

        std::vector<const char*> token_names;
        token_names.push_back("$");
        for (const auto& tok_iter : tokens)
        {
            token_names.push_back(tok_iter.first.c_str());
        }
        for (const auto& grm_iter : grammars)
        {
            token_names.push_back(grm_iter.first.c_str());
        }
        for (const auto& grm_iter : generated_grammars)
        {
            token_names.push_back(grm_iter.first.c_str());
        }

        for (const auto& iter : s1.left)
        {
            auto it = tokens.find(iter->value);
            if (it == tokens.end())
            {
                context.emit_error(iter, "Undefined token");
            }
            else if (precedence_mapping.find(it->second.id) != precedence_mapping.end())
            {
                context.emit_error(iter, "Already defined precedence for '%s'", iter->value.c_str());
            }
            else
            {
                precedence_mapping.emplace(it->second.id, PRECEDENCE_LEFT);
            }
        }
        for (const auto& iter : s1.right)
        {
            auto it = tokens.find(iter->value);
            if (it == tokens.end())
            {
                context.emit_error(iter, "Undefined token");
            }
            else if (precedence_mapping.find(it->second.id) != precedence_mapping.end())
            {
                context.emit_error(iter, "Already defined precedence for '%s'", iter->value.c_str());
            }
            else
            {
                precedence_mapping.emplace(it->second.id, PRECEDENCE_RIGHT);
            }
        }

        GrammarParser parser = {
                .ascii_mappings = nullptr,
                .grammar_rules = grammar_rules.data(),
                .token_names = token_names.data(),
                .parser_error = nullptr,
                .parser_reduce = nullptr,
                .grammar_n = static_cast<tok_t>(grammar_rules.size()),
                .token_n = static_cast<tok_t>(token_names.size()),
                .action_token_n = static_cast<tok_t>(tokens.size() + 1)
        };

        cc = up<parsergen::CanonicalCollection>(
                new parsergen::CanonicalCollection(
                        &parser, &context,
                        rule_positions.data()));

        if (context.has_errors())
        {
            context.emit_message(nullptr, "Failed to initialize canonical collection");
            return;
        }

        const std::string& parser_type_s = options["parser_type"];
        parser_t parser_type;
        if (parser_type_s == "LALR(1)")
        {
            parser_type = LALR_1;
        }
        else if (parser_type_s == "CLR(1)")
        {
            parser_type = CLR_1;
        }
        else
        {
            context.emit_warning(
                    nullptr,
                    "Unknown parser type '%s'. Defaulting to LALR(1)",
                    parser_type_s.c_str());
            parser_type = LALR_1;
        }

        cc->resolve(parser_type);
        if (context.has_errors())
        {
            context.emit_message(nullptr, "Failed to resolve canonical collection");
            return;
        }

        precedence_table = up<uint8_t[]>(new uint8_t[tokens.size()]);
        for (const auto &mapping : precedence_mapping)
        {
            precedence_table[mapping.first] = mapping.second;
        }

        parsing_table = up<uint32_t[]>(new uint32_t[cc->table_size()]);
        auto gen_error = cc->generate(parsing_table.get(), precedence_table.get());
    }

    void Compiler::do_stage(const std::function<void()>& stage_cb, int error_code)
    {
        if (error || context.has_errors()) return;

        stage_cb();
        context.put_messages();

        if (context.has_errors())
        {
            error = error_code;
        }
    }

    int Compiler::compile(const std::string& file_path)
    {
        clear();

        // Load the entry point file into the main AST
        do_stage([=](){
            ast = load_file(file_path);
        }, 1);

        // Perform the compiler stages
        do_stage(std::bind(&Compiler::do_s0, this), 2);
        do_stage(std::bind(&Compiler::do_s1, this), 3);
        do_stage(std::bind(&Compiler::do_s2, this), 4);
        do_stage(std::bind(&Compiler::do_s3, this), 5);
        do_stage(std::bind(&Compiler::do_s4, this), 6);
        do_stage(std::bind(&Compiler::do_s5, this), 7);
        do_stage(std::bind(&Compiler::do_s6, this), 8);
        do_stage(std::bind(&Compiler::do_s7, this), 9);

        return error;
    }

    Compiler::Compiler()
    : context(file_inputs), error(0), options({
        {"namespace", "ast"},
        {"ast_class", "Ast"},
        {"token_class", "Token"},
        {"parser_type", "LALR(1)"},
    }), num_reserved_tokens(0)
    {
    }

    void Compiler::clear()
    {
        cc = nullptr;
        ast = nullptr;
        types.clear();
        tokens.clear();
        file_inputs.clear();
        ast_inputs.clear();
        grammars.clear();
        productions.clear();

        processed_imports.clear();

        generated_tokens.clear();
        generated_enums.clear();
        generated_vectors.clear();

        s1.start = nullptr;
        s1.left.clear();
        s1.right.clear();
        s1.top.clear();
        s1.include.clear();
        s1.bottom.clear();

        error = 0;
        num_reserved_tokens = 0;
        reserved_tokens.clear();
    }

    BaseGrammar &Compiler::generate_grammar(const TokenPosition& position, Type* type)
    {
        std::string name = "GENERATED_" + std::to_string(productions.size());
        generated_grammars.emplace(
                name, BaseGrammar(position, name, type));
        return generated_grammars.at(name);
    }
}
