
#include <cassert>
#include "compiler.h"

extern "C" {
neoast::ast::CompilerCompiler*
cc_parse_len(int context, void* error_ctx, void* buffers_,
             const char* input, uint32_t input_len);
void* cc_allocate_buffers();

void cc_free_buffers(void* self);
}

namespace neoast
{
    template<typename T, typename K>
    static const T* up_cast(const ast::up<K> &from, bool assertion = true)
    {
        const auto* casted = dynamic_cast<const T*>(from.get());

        if (assertion)
        {
            assert(casted);
        }

        return casted;
    }

    template<typename T, typename K>
    static T* up_cast(ast::up<K> &from, bool assertion = true)
    {
        auto* casted = dynamic_cast<T*>(from.get());

        if (assertion)
        {
            assert(casted);
        }

        return casted;
    }

    template<typename T>
    void for_each_item(
            ast::CompilerCompiler* ast,
            std::function<void(const T*, std::list<up<Ast>>::iterator &iter)> cb)
    {
        for (auto iter = ast->items.begin();
             iter != ast->items.end();
             ++iter)
        {
            const auto* casted = up_cast<T>(*iter, false);
            if (casted)
            {
                cb(casted, iter);
            }
        }
    }

    template<typename T>
    void for_each_item(
            ast::CompilerCompiler* ast,
            std::function<void(const T*)> cb)
    {
        for (const auto &item: ast->items)
        {
            const auto* casted = up_cast<T>(item, false);
            if (casted)
            {
                cb(casted);
            }
        }
    }

    template<typename T>
    void for_each_item(
            ast::CompilerCompiler* ast,
            std::function<void(T*)> cb)
    {
        for (auto &item: ast->items)
        {
            auto* casted = up_cast<T>(item, false);
            if (casted)
            {
                cb(casted);
            }
        }
    }

    void for_each_atom(const ast::Expr* expr,
                       const std::function<void(const ast::Atom*)> &cb)
    {
        if (!expr) return;

        // The atom can be a variety of types
        auto* op_expr = dynamic_cast<const ast::OpExpr*>(expr);
        auto* expansion = dynamic_cast<const ast::Expansion*>(expr);
        auto* or_expr = dynamic_cast<const ast::OrExpr*>(expr);
        auto* atom = dynamic_cast<const ast::Atom*>(expr);

        if (expansion)
        {
            for (const auto& iter : expansion->all)
            {
                for_each_atom(iter.get(), cb);
            }
        }
        else if (op_expr)
        {
            for_each_atom(op_expr->inner.get(), cb);
        }
        else if (or_expr)
        {
            for_each_atom(or_expr->left.get(), cb);
            for_each_atom(or_expr->right.get(), cb);
        }
        else if (atom)
        {
            cb(atom);
        }
        else
        {
            throw Exception("Expected token to be ast::Expr or ast::Atom");
        }
    }

    void for_each_atom(std::list<up<ast::Expr>>& exprs,
                       const std::function<void(const ast::Atom*)> &cb)
    {
        for (auto& iter : exprs)
        {
            for_each_atom(iter.get(), cb);
        }
    }

    TokenPosition merge_positions(const TokenPosition& t1,
                                  const TokenPosition& t2)
    {
        return {
                t1.line,
                t1.col,
                (t1.line == t2.line) ? (t2.col + t2.len) - t1.col : t1.len,
                t1.context
        };
    }

    TokenPosition get_position_from_expr(const ast::Expr* expr)
    {
        auto* op_expr = dynamic_cast<const ast::OpExpr*>(expr);
        auto* expansion = dynamic_cast<const ast::Expansion*>(expr);
        auto* or_expr = dynamic_cast<const ast::OrExpr*>(expr);
        auto* atom = dynamic_cast<const ast::Atom*>(expr);

        if (atom)
        {
            if (atom->name)
            {
                return merge_positions(
                        *atom->value->token,
                        *atom->name);
            }
            else
            {
                return {
                    atom->value->token->line,
                    atom->value->token->col,
                    atom->value->token->len,
                    atom->value->token->context,
                };
            }
        }
        else if (op_expr)
        {
            return get_position_from_expr(op_expr->inner.get());
        }
        else if (expansion)
        {
            TokenPosition t1 = get_position_from_expr(expansion->all.front().get());
            TokenPosition t2 = get_position_from_expr(expansion->all.back().get());

            return merge_positions(t1, t2);
        }
        else if (or_expr)
        {
            return merge_positions(
                    get_position_from_expr(or_expr->left.get()),
                    get_position_from_expr(or_expr->right.get()));
        }
        else
        {
            assert(0 && "Invalid expression");
        }
    }

    TokenPosition get_position_from_expr(const std::list<up<ast::Expr>>& exprs)
    {
        TokenPosition t1 = get_position_from_expr(exprs.front().get());
        TokenPosition t2 = get_position_from_expr(exprs.back().get());

        return merge_positions(t1, t2);
    }

    ast::CompilerCompiler* Compiler::load_file(const std::string &path)
    {
        // TODO(tumbar) We can use member level buffers instead to save alloc time
        auto buf = std::unique_ptr<void, std::function<void(void*)>>(
                cc_allocate_buffers(), cc_free_buffers);

        int new_context = static_cast<int>(file_inputs.size());

        try
        {
            file_inputs.emplace_back(path);
        }
        catch (Exception &e)
        {
            context.emit_error(nullptr, e.what());
            return nullptr;
        }

        auto &input = file_inputs.back();

        auto* parsed = cc_parse_len(
                new_context,
                &input, buf.get(),
                input.get_content(),
                input.size()
        );

        ast_inputs.emplace_back(
                std::unique_ptr<ast::CompilerCompiler>(parsed));

        if (!parsed)
        {
            input.emit_error(nullptr, "Failed to parse file");
        }

        return parsed;
    }

    void Compiler::do_s0()
    {
        // Recursively import things until we have nothing left to import
        bool dirty = true;
        while (dirty)
        {
            dirty = false;
            for_each_item<ast::Stmt>(
                    ast,
                    [=, &dirty](const ast::Stmt* stmt,
                                std::list<up<Ast>>::iterator &iter)
                    {
                        if (stmt->type == ast::Stmt::IMPORT)
                        {
                            // Skip a previously processed import statement
                            if (processed_imports.find(stmt) != processed_imports.end())
                            {
                                return;
                            }

                            const auto* import_literal =
                                    up_cast<ast::Token>(stmt->value);

                            dirty = true;

                            // TODO(tumbar) Make sure we are not recursively parsing file loop
                            auto* loaded_file = load_file(import_literal->value);

                            if (loaded_file)
                            {
                                // Place all AST items our main ast at the import position
                                ast->items.splice(iter, loaded_file->items);
                            }
                            else
                            {
                                context.emit_error(import_literal, "Failed to process import");
                            }

                            processed_imports.emplace(stmt);
                        }
                    }
            );
        }
    }

    void Compiler::do_s1()
    {
        for_each_item<ast::KVStmt>(
                ast,
                [=](const ast::KVStmt* kv_stmt)
                {
                    switch (kv_stmt->type)
                    {
                        case ast::KVStmt::NONE:
                            break;
                        case ast::KVStmt::OPTION:
                        {
                            if (options.find(kv_stmt->key->value) == options.end())
                            {
                                // Not a defined option, warn but keep it
                                context.emit_warning(kv_stmt->key.get(),
                                                     "Unknown option key '%s'",
                                                     kv_stmt->key->value.c_str());
                            }

                            // Add the option to the umap
                            options[kv_stmt->key->value] = up_cast<ast::Token>(kv_stmt->value)->value;
                        }
                            break;
                        case ast::KVStmt::TOKEN:
                            for (const auto &token: up_cast<ast::Tokens>(kv_stmt->value)->tokens)
                            {
                                // Make sure this token is not a duplicate
                                auto it = tokens.find(token->value);
                                if (it != tokens.end())
                                {
                                    context.emit_error(token.get(), "Duplicate definition of token");
                                    context.emit_note(&it->second.position, "First definition is here");
                                    continue;
                                }

                                tokens.emplace(token->value, std::move(Token(*token, kv_stmt->key.get())));
                            }
                            break;
                        case ast::KVStmt::MACRO:
                            break;
                    }
                }
        );

        for_each_item<ast::Stmt>(
                ast,
                [=](const ast::Stmt* stmt)
                {
                    switch (stmt->type)
                    {
                        case ast::Stmt::NONE:
                            // These should all be processed already
                            assert(0);
                        case ast::Stmt::START:
                            s1.start = up_cast<ast::Token>(stmt->value);
                            break;
                        case ast::Stmt::RIGHT:
                            for (const auto &token: up_cast<ast::Tokens>(stmt->value)->tokens)
                                s1.right.emplace_back(token.get());
                            break;
                        case ast::Stmt::LEFT:
                            for (const auto &token: up_cast<ast::Tokens>(stmt->value)->tokens)
                                s1.left.emplace_back(token.get());
                            break;
                        case ast::Stmt::TOP:
                            s1.top.emplace_back(up_cast<ast::Token>(stmt->value));
                            break;
                        case ast::Stmt::INCLUDE:
                            s1.include.emplace_back(up_cast<ast::Token>(stmt->value));
                            break;
                        case ast::Stmt::BOTTOM:
                            s1.bottom.emplace_back(up_cast<ast::Token>(stmt->value));
                            break;
                        default:
                            break;
                    }
                });

        for_each_item<ast::ManualDeclaration>(
                ast,
                [=](const ast::ManualDeclaration* decl)
                {
                    types.emplace(decl->name->value, new ManualDeclaration(*decl));
                });
    }

    void Compiler::do_s2()
    {
        // Create the builtin declarations first
        types.emplace(options["ast_class"], new AstType(options["ast_class"]));
        types.emplace(options["token_class"], new TokenType(options["token_class"]));

        // Add the tokens not explicitly defined by '%token'
        for_each_item<ast::TokenRule>(
                ast,
                [=](const ast::TokenRule* t_rule)
                {
                    // Make sure this token is not a duplicate
                    auto it = tokens.find(t_rule->name->value);
                    if (it != tokens.end())
                    {
                        context.emit_error(t_rule->name.get(), "Duplicate definition of token");
                        context.emit_note(&it->second.position, "First definition is here");
                        return;
                    }

                    tokens.emplace(t_rule->name->value,
                                   Token(*t_rule->name,
                                         types.at(options["token_class"]).get(),
                                         t_rule->name->value,
                                         t_rule->regex->value));
                });

        // Make through each grammar rule and add all the literals
        for_each_item<ast::Grammar>(
                ast,
                [=](const ast::Grammar* g_rule)
                {
                    // Look for literals in the productions
                    for (const auto &prod: g_rule->productions)
                    {
                        for_each_atom(
                                prod->rule, [=](const ast::Atom* atom)
                                {
                                    if (atom->value->type == ast::Value::LITERAL ||
                                        atom->value->type == ast::Value::REGEX)
                                    {
                                        std::string token_name;
                                        if (extract_token_name_from_literal(
                                                atom->value->token->value, token_name))
                                        {
                                            // This literal has already been processed
                                            return;
                                        }

                                        Token t(*atom->value->token,
                                                types.at(options["token_class"]).get(),
                                                token_name,
                                                // Place word boundaries around the literal
                                                // ...or if regex, keep the original rule as our regex
                                                // TODO(tumbar) Better sanitize of literals to make them regex ready
                                                (atom->value->type == ast::Value::LITERAL) ?
                                                atom->value->token->value + "\\b"
                                                                                           : atom->value->token->value);
                                        t.use_regex_for_c_name = true;
                                        tokens.emplace(
                                                atom->value->token->value,
                                                std::move(t));
                                    }
                                });
                    }
                }
        );
    }

    void Compiler::do_s3()
    {
        for_each_item<ast::Grammar>(
                ast,
                [=](ast::Grammar* grammar)
                {
                    // Make sure this grammar name doesn't overlap with any
                    // tokens or previously defined grammars
                    auto token_it = tokens.find(grammar->name->value);
                    if (token_it != tokens.end())
                    {
                        context.emit_error(grammar->name.get(), "Overlapping grammar name");
                        context.emit_note(&token_it->second.decl, "First definition as a token");
                        return;
                    }

                    auto grammar_it = grammars.find(grammar->name->value);
                    if (grammar_it != grammars.end())
                    {
                        context.emit_error(grammar->name.get(), "Overlapping grammar name");
                        context.emit_note(grammar_it->second.decl.name.get(), "First definition is here");
                        return;
                    }

                    grammars.emplace(grammar->name->value, *grammar);
                    Grammar &cc_grammar = grammars.at(grammar->name->value);

                    if (grammar->generates)
                    {
                        // If we are generating a class or enum, make sure it doesn't exist yet
                        // If we are generating a vector of a class or enum, make sure that,
                        //   it has already been defined
                        auto it = types.find(grammar->generates->name->value);
                        switch (grammar->generates->type)
                        {
                            case ast::ENUM:
                            case ast::CLASS:
                                if (it != types.end())
                                {
                                    context.emit_error(
                                            grammar->generates->name.get(),
                                            "Duplicate definition of generated type"
                                    );

                                    auto* positioned = up_cast<Positioned>(it->second, false);
                                    if (positioned)
                                    {
                                        context.emit_note(&positioned->position, "First definition is here");
                                    }
                                    else
                                    {
                                        context.emit_error(nullptr,
                                                           "Definition was automatically created by the compiler");
                                    }

                                    return;
                                }
                                break;
                            case ast::VECTOR:
                                if (it == types.end())
                                {
                                    context.emit_error(
                                            grammar->generates->name.get(),
                                            "'vector' definition requires already defined class or enum");
                                    return;
                                }
                                break;
                            default:
                            case ast::DeclarationType_NONE:
                                context.emit_error(grammar->generates->name.get(),
                                                   "DeclarationType_NONE is unexpected in type declaration");
                                return;
                        }

                        if (grammar->generates->type == ast::ENUM ||
                            grammar->generates->type == ast::VECTOR)
                        {
                            if (grammar->generates->extends)
                            {
                                context.emit_warning(grammar->generates->extends.get(),
                                                     "Only 'class' can extend types");
                            }

                            if (grammar->generates->extra_code)
                            {
                                context.emit_warning(grammar->generates->extra_code.get(),
                                                     "Only 'class' define extra code");
                            }
                        }

                        // Add new type to the compiler registry
                        Type* generated_type = nullptr;
                        switch (grammar->generates->type)
                        {
                            case ast::DeclarationType_NONE:
                                assert(0);
                            case ast::CLASS:
                                generated_type = new AstClass(*grammar->generates);
                                types.emplace(grammar->generates->name->value, generated_type);
                                break;
                            case ast::ENUM:
                                generated_type = new AstEnum(*grammar->name);
                                types.emplace(grammar->generates->name->value, generated_type);
                                break;
                            case ast::VECTOR:
                                // The vector type is not a real generated type
                                // It is just a wrapper around the element
                                generated_vectors.emplace_back(
                                        *grammar->generates->name,
                                        (*it).second.get());
                                generated_type = &generated_vectors.back();
                                break;
                        }

                        cc_grammar.type = generated_type;
                    }
                });

        // Resolve type inheritance
        for (auto &type_pair: types)
        {
            type_pair.second->resolve_inheritance(*this);
        }

        // Resolve grammar types
        for (auto &grammar: grammars)
        {
            if (!grammar.second.type)
            {
                // This grammar does not generate a type
                // This means we must resolve the type from a generated type
                assert(!grammar.second.decl.generates);
                assert(grammar.second.decl.type);

                auto type_it = types.find(grammar.second.decl.type->value);
                if (type_it == types.end())
                {
                    context.emit_error(grammar.second.decl.type.get(),
                                       "Type '%s' has not been declared",
                                       grammar.second.decl.type->value.c_str());
                    continue;
                }

                grammar.second.type = type_it->second.get();
            }
        }

        // Resolve token types
        for (auto &token: tokens)
        {
            if (!token.second.type)
            {
                assert(token.second.type_decl);
                auto type_it = types.find(token.second.type_decl->value);
                if (type_it == types.end())
                {
                    context.emit_error(token.second.type_decl,
                                       "Type '%s' has not been declared",
                                       token.second.type_decl->value.c_str());
                    continue;
                }

                token.second.type = type_it->second.get();
            }
        }
    }

    void Compiler::do_s4()
    {
        for (auto &iter: grammars)
        {
            auto &grammar = iter.second;
            assert(grammar.type);

            auto* ast_class = dynamic_cast<AstClass*>(grammar.type);
            auto* ast_enum = dynamic_cast<AstEnum*>(grammar.type);

            AstEnum* generated_enum = nullptr;
            for (const auto &prod: grammar.decl.productions)
            {
                // Build members for classes
                // Build enumerates for enums
                if (ast_class)
                {
                    // Build a class type enumerate if needed
                    if (prod->alias)
                    {
                        if (!generated_enum)
                        {
                            TokenPosition pos_null = {0, 0, 0, 0};
                            generated_tokens.emplace_back(&pos_null, "Type");
                            generated_enums.emplace_back(generated_tokens.back());
                            generated_enum = &generated_enums.back();

                            // Add a type member
                            // We only need this once
                            ast_class->set_type_enumerate(generated_enum);
                        }

                        generated_enum->add_enumerate(prod->alias->value);
                    }

                    for_each_atom(
                            prod->rule, [=](const ast::Atom* atom)
                            {
                                std::string member_name;
                                Type* member_type;

                                if (atom->name)
                                {
                                    member_name = atom->name->value;
                                }
                                else
                                {
                                    if (atom->value->type == ast::Value::TOKEN)
                                    {
                                        member_name = atom->value->token->value;
                                    }
                                    else
                                    {
                                        // Token is either REGEX or LITERAL
                                        // Since it was not given a name we assume
                                        // the user doesn't want it in the AST
                                        return;
                                    }
                                }

                                if (atom->value->type == ast::Value::TOKEN)
                                {
                                    // Look for this token's type definition
                                    auto tok_it = tokens.find(atom->value->token->value);
                                    if (tok_it != tokens.end())
                                    {
                                        // This is a simple token
                                        member_type = types.at(options["token_class"]).get();
                                    }
                                    else
                                    {
                                        // This must be another grammar rule
                                        auto grammar_it = grammars.find(atom->value->token->value);
                                        if (grammar_it != grammars.end())
                                        {
                                            member_type = grammar_it->second.type;
                                        }
                                        else
                                        {
                                            context.emit_error(atom->value->token.get(), "Undeclared token");
                                            return;
                                        }
                                    }
                                }
                                else
                                {
                                    member_type = types.at(options["token_class"]).get();
                                }

                                if (member_name == "this")
                                {
                                    // Make sure "this" matches the grammar's type
                                    if (member_type != grammar.type)
                                    {
                                        context.emit_error(
                                                atom->value->token.get(),
                                                "Types of Grammar (%s) type and 'this' (%s) must match",
                                                grammar.type->name().c_str(),
                                                member_type->name().c_str());
                                    }

                                    // We don't want to add 'this' as a member since its special
                                }
                                else
                                {
                                    ast_class->add_member(member_name, member_type, *this);
                                }
                            });
                }
                else if (ast_enum)
                {
                    if (prod->action)
                    {
                        context.emit_error(prod->action.get(),
                                           "Enums productions cannot have actions");
                    }

                    if (prod->rule.size() != 1)
                    {
                        auto pos = get_position_from_expr(prod->rule);
                        context.emit_error(&pos, "All enum productions must be a single token");
                        continue;
                    }

                    auto a = up_cast<const ast::Atom>(prod->rule.front());
                    if (a->name)
                    {
                        context.emit_warning(a->name.get(), "Member names are ignored in enum declaration");
                    }

                    if (!prod->alias)
                    {
                        context.emit_error(a->value->token.get(),
                                           "All enum productions must have an '-> ALIAS'");
                        continue;
                    }

                    ast_enum->add_enumerate(a->value->token->value);
                }
            }
        }
    }

    std::string Compiler::get_reserved_token(int token_id)
    {
        return "__RESERVED_TOKEN_" + std::to_string(token_id);
    }

    bool Compiler::extract_token_name_from_literal(const std::string &literal,
                                                   std::string &token_name)
    {
        auto it = reserved_tokens.find(literal);
        int tok_id = (it == reserved_tokens.end()) ? num_reserved_tokens++ : it->second;

        token_name = get_reserved_token(tok_id);
        return it != reserved_tokens.end();
    }
}
