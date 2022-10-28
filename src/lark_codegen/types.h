
#ifndef NEOAST_TYPES_H
#define NEOAST_TYPES_H

#include <string>
#include <unordered_set>

#include "common.h"
#include <neoast.h>

#include "ast.h"

namespace neoast
{
    class Type;
    class Compiler;

    struct Positioned
    {
        TokenPosition position;
        explicit Positioned(const TokenPosition& position_)
                : position(position_) {}
    };

    class LRItem : public Positioned
    {
    public:
        int id = -1;
        const std::string name;
        Type* type;

        LRItem(const TokenPosition& position,
               std::string name_,
               Type* type_);

        virtual ~LRItem() = default;
    };

    class Type
    {
    public:
        virtual ~Type() = default;
        virtual void resolve_inheritance(Compiler& c);

        const Type* common_base(const Type* other) const;
        Type* common_base(const Type* other);
        virtual const Type* base_class() const;
        virtual Type* base_class();

        virtual std::string name() const = 0;
        virtual std::string union_type() const { return name(); };
        virtual std::string member_type() const { return name(); };
        virtual std::string member_default() const = 0;

        virtual std::string get_definition(Compiler& c) const = 0;
        virtual std::string get_construction(Compiler& c, const std::string& var_name) const = 0;
        virtual std::string get_union_delete(Compiler& c, const std::string& var_name) const;
        virtual std::string set_member(Compiler& c, const std::string& var_name, const std::string& member_name, const std::string& value) const;
        virtual std::string move_into(Compiler& c, const std::string& destination, const std::string& value) const;
    };

    class AstUniquePtrBase : public Type
    {
    public:
        std::string union_type() const override;
        std::string member_type() const override;
        std::string member_default() const override;

        std::string get_construction(Compiler& c, const std::string& var_name) const override;
        std::string get_union_delete(Compiler& c, const std::string& var_name) const override;
        std::string move_into(Compiler &c, const std::string &destination, const std::string &value) const override;
    };

    class TokenType : public AstUniquePtrBase
    {
        std::string m_name;

    public:
        explicit TokenType(std::string name_);

        std::string name() const override;
        std::string union_type() const override;
        std::string member_type() const override;
        std::string member_default() const override;

        std::string get_definition(Compiler& c) const override;
        std::string set_member(Compiler& c, const std::string& var_name, const std::string& member_name, const std::string& value) const override;
    };

    class AstType : public AstUniquePtrBase
    {
        std::string m_name;

    public:
        explicit AstType(std::string name_);
        std::string name() const override;

        std::string get_definition(Compiler& c) const override;
        std::string set_member(Compiler& c, const std::string& var_name, const std::string& member_name, const std::string& value) const override;
    };

    class AstEnum;
    class AstClass : public AstUniquePtrBase, public Positioned
    {
        const ast::Declaration& decl;
        Type* inherits_from;

        const AstEnum* type_enumerate;

        // Keep track of members and where they are referenced
        umap<std::string, Type*> members;

    public:
        explicit AstClass(const ast::Declaration& decl_);
        void resolve_inheritance(Compiler& c) override;

        void set_type_enumerate(const AstEnum* type_enumerate);

        const Type* base_class() const override;
        Type* base_class() override;

        std::string name() const override;

        bool has_member(const std::string& name) const;
        const Type* get_member(const std::string& name) const;
        void add_member(const std::string& name,
                        Type* type,
                        Compiler& c);

        std::string get_definition(Compiler& c) const override;
        std::string set_member(Compiler& c,
                               const std::string& var_name,
                               const std::string& member_name,
                               const std::string& value) const override;
    };

    class AstEnum : public Type, public Positioned
    {
        ast::Token& decl;
        std::unordered_set<std::string> enumerations;

    public:
        explicit AstEnum(ast::Token& name_);
        void add_enumerate(const std::string& enumerate_name);

        std::string name() const override;
        std::string member_default() const override;

        std::string get_definition(Compiler& c) const override;
        std::string get_construction(Compiler& c, const std::string& var_name) const override;
        std::string get_union_delete(Compiler &c, const std::string &var_name) const override;
    };

    class AstVector : public Type, public Positioned
    {
        ast::Token m_name_token;
        Type* m_element_type;

    public:
        explicit AstVector(ast::Token name_token_,
                           Type* element_type_);

        void resolve_inheritance(Compiler &c) override;

        Type* element_type() const;

        std::string name() const override;

        std::string member_type() const override;
        std::string member_default() const override;
        std::string get_construction(Compiler &c, const std::string &var_name) const override;
        std::string get_definition(Compiler& c) const override;
        std::string get_union_delete(Compiler &c, const std::string &var_name) const override;
        std::string move_into(Compiler &c, const std::string &destination, const std::string &value) const override;
    };

    class ManualDeclaration : public Type
    {
        const ast::ManualDeclaration& decl;

    public:
        explicit ManualDeclaration(const ast::ManualDeclaration& decl_);

        std::string name() const override;
        std::string member_default() const override;

        std::string get_definition(Compiler& c) const override;
        std::string get_construction(Compiler& c, const std::string& var_name) const override;
        std::string get_union_delete(Compiler &c, const std::string &var_name) const override;
    };
}

#endif //NEOAST_TYPES_H
