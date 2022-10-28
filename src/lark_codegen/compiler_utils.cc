
#include "compiler.h"

namespace neoast
{
    Token::Token(const ast::Token &decl_, Type* type_, std::string name_, std::string regex_)
            : LRItem(decl_, std::move(name_), type_), decl(decl_),
              type_decl(nullptr),
              generates_rule(true),
              regex(std::move(regex_))
    {
    }

    Token::Token(const ast::Token &decl_,
                 const ast::Token* type_decl_)
            : LRItem(decl_, decl_.value, /* resolve later */ nullptr), decl(decl_),
              type_decl(type_decl_),
              generates_rule(false)
    {
    }

    BaseGrammar::BaseGrammar(const TokenPosition &position_,
                             std::string name_,
                             Type* type)
            : LRItem(position_, std::move(name_), type)
    {
    }

    Grammar::Grammar(const ast::Grammar &decl_)
            : BaseGrammar(*decl_.name, decl_.name->value,
            /* resolved in compiler stage */ nullptr),
              decl(decl_)
    {
    }

    ProductionTree::ProductionTree()
    {
        branches.emplace_back();
    }

    void ProductionTree::branch(const ProductionTree &left_tree,
                                const ProductionTree &right_tree)
    {
        // Create two distinct set of branches
        ProductionTree right_tree_prod(*this);
        ProductionTree left_tree_prod(std::move(*this));
        branches = std::list<Branch>();

        // Push each respective option to each branch
        left_tree_prod.push(left_tree);
        right_tree_prod.push(right_tree);

        // Merge these branches into the mainline
        branches.splice(branches.end(), left_tree_prod.branches);
        branches.splice(branches.end(), right_tree_prod.branches);
    }

    void ProductionTree::push(const ProductionTree &tree)
    {
        std::list<ProductionTree> new_trees;
        for (const auto &new_branch: tree)
        {
            // Create a new tree with this tree as a starting point
            new_trees.emplace_back(*this);

            // Add every new token to every generated branch
            for (auto &branch: new_trees.back())
            {
                for (const auto &token: new_branch)
                {
                    branch.emplace_back(token);
                }
            }
        }

        branches.clear();
        for (auto& tree_iter : new_trees)
        {
            for (auto& branch_iter : tree_iter)
            {
                branches.push_back(branch_iter);
            }
        }
    }

    void ProductionTree::push(LRItem* item)
    {
        for (auto &branch: branches)
        {
            branch.push_back(item);
        }
    }

    std::list<ProductionTree::Branch>::iterator ProductionTree::begin()
    {
        return branches.begin();
    }

    std::list<ProductionTree::Branch>::iterator ProductionTree::end()
    {
        return branches.end();
    }

    std::list<ProductionTree::Branch>::const_iterator ProductionTree::begin() const
    {
        return branches.begin();
    }

    std::list<ProductionTree::Branch>::const_iterator ProductionTree::end() const
    {
        return branches.end();
    }


    Type* get_expr_type(const ast::Expr* expr, Compiler& c)
    {
        auto* op_expr = dynamic_cast<const ast::OpExpr*>(expr);
        auto* expansion = dynamic_cast<const ast::Expansion*>(expr);
        auto* or_expr = dynamic_cast<const ast::OrExpr*>(expr);
        auto* atom = dynamic_cast<const ast::Atom*>(expr);

        if (op_expr)
        {
            Type* underlying_type = get_expr_type(op_expr->inner.get(), c);
            if (!underlying_type)
            {
                return nullptr;
            }

            TokenPosition pos = get_position_from_expr(op_expr->inner.get());

            switch(op_expr->op)
            {
                case ast::Op_NONE:
                    assert(0);
                case ast::ZERO_OR_ONE:
                    // Zero means the default constructor
                    // One is just the standard type
                    return underlying_type;
                case ast::ZERO_OR_MORE:
                case ast::ONE_OR_MORE:
                    c.generated_vectors.emplace_back(
                            std::move(ast::Token(&pos, underlying_type->name() + "_Vector")),
                            underlying_type);
                    return &c.generated_vectors.back();
            }
        }
        else if (expansion)
        {
            return nullptr;
        }
        else if (or_expr)
        {
            Type* left_type = get_expr_type(or_expr->left.get(), c);
            Type* right_type = get_expr_type(or_expr->right.get(), c);

            if (!left_type || !right_type)
            {
                return nullptr;
            }

            return left_type->common_base(right_type);
        }
        else
        {
            switch(atom->value->type)
            {
                case ast::Value::TOKEN:
                {
                    auto tok_it = c.tokens.find(atom->value->token->value);
                    if (tok_it != c.tokens.end())
                    {
                        // This is a simple token
                        return c.types.at(c.options["token_class"]).get();
                    }
                    else
                    {
                        // This must be another grammar rule
                        auto grammar_it = c.grammars.find(atom->value->token->value);
                        if (grammar_it != c.grammars.end())
                        {
                            return grammar_it->second.type;
                        }
                        else
                        {
                            c.context.emit_error(atom->value->token.get(), "Undeclared token");
                            return nullptr;
                        }
                    }
                }
                case ast::Value::LITERAL:
                case ast::Value::REGEX:
                    return c.types.at(c.options["token_class"]).get();
                default:
                    assert(0);
            }
        }

        return nullptr;
    }

}
