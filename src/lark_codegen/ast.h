
#ifndef NEOAST_AST_H
#define NEOAST_AST_H

#include <neoast.h>
#include <string>
#include <utility>
#include <list>
#include <memory>

#include <common/context.h>

namespace neoast
{
    namespace ast
    {
        template<typename T>
        using up = std::unique_ptr<T>;

        struct Token : public Ast, public TokenPosition
        {
            std::string value;

            Token(const TokenPosition* position_, std::string value_)
                    : value(std::move(value_)), TokenPosition(*position_)
            {
            }
        };

        struct Tokens : public Ast
        {
            std::list<up<Token>> tokens;
        };

        struct KVStmt : public Ast
        {
            enum Type
            {
                NONE,
                OPTION,
                TOKEN,
                MACRO
            };

            Type type = NONE;
            up<Token> key = nullptr;
            up<Ast> value = nullptr;
        };

        struct Stmt : public Ast
        {
            enum Type
            {
                NONE,
                START,
                RIGHT,
                LEFT,
                TOP,
                INCLUDE,
                BOTTOM,
                IMPORT
            };

            Type type = NONE;
            up<Ast> value = nullptr;
        };

        struct Value : public Ast
        {
            enum Type
            {
                NONE,
                TOKEN,
                LITERAL,
                REGEX,
            };

            Type type = NONE;
            up<Token> token = nullptr;
        };

        enum Op
        {
            Op_NONE,
            ZERO_OR_ONE,
            ZERO_OR_MORE,
            ONE_OR_MORE,
        };

        struct Expr : public Ast
        {
        };

        struct OpExpr : public Expr
        {
            up<Expr> inner = nullptr;
            Op op = Op_NONE;
//            int n1 = -1;
//            int n2 = -1;
        };

        struct Atom : public Expr
        {
            up<Value> value = nullptr;
            up<Token> name = nullptr;
        };

        struct Expansion : public Expr
        {
            std::list<up<Expr>> all;
        };

        struct OrExpr : public Expr
        {
            up<Expr> left = nullptr;
            up<Expr> right = nullptr;
        };

        struct Production : public Ast
        {
            std::list<up<Expr>> rule;
            up<Token> alias = nullptr;
            up<Token> action = nullptr;
        };

        struct Vector_Production : public Ast
        {
            std::list<up<Production>> all;
        };

        struct ManualDeclaration : public Ast
        {
            up<ast::Token> name = nullptr;
            up<ast::Token> c_typename = nullptr;
            up<ast::Token> c_default = nullptr;
            up<ast::Token> c_delete = nullptr;
        };

        enum DeclarationType
        {
            DeclarationType_NONE,
            CLASS,
            ENUM,
            VECTOR,
        };

        struct Declaration : public Ast
        {
            DeclarationType type = DeclarationType_NONE;
            up<Token> name = nullptr;
            up<Token> extends = nullptr;
            up<Token> extra_code = nullptr;
        };

        struct Grammar : public Ast
        {
            up<Declaration> generates = nullptr;
            up<Token> type = nullptr;
            up<Token> name = nullptr;
            std::list<up<Production>> productions;
        };

        struct TokenRule : public Ast
        {
            up<Token> name = nullptr;
            up<Token> regex = nullptr;
        };

        struct LexerRule : public Ast
        {
            up<Token> regex = nullptr;
            up<Token> action = nullptr;
        };

        struct Vector_LexerRule : public Ast
        {
            std::list<up<LexerRule>> all;
        };

        struct LexerState : public Ast
        {
            up<Token> name;
            std::list<up<LexerRule>> rules;
        };

        struct Vector_Ast : public Ast
        {
            std::list<up<Ast>> all;
        };

        struct CompilerCompiler : public Ast
        {
            std::list<up<Ast>> items;
        };
    }
}

#endif //NEOAST_AST_H
