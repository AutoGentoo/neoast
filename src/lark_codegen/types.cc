#include <sstream>
#include <util/util.h>
#include <lark_codegen/types.h>
#include <lark_codegen/compiler.h>

namespace neoast
{
    LRItem::LRItem(const TokenPosition& position,
                   std::string name_, Type* type_)
            : Positioned(position), name(std::move(name_)), type(type_)
    {
    }

    AstClass::AstClass(const ast::Declaration &decl_)
            : Positioned(*decl_.name), decl(decl_), inherits_from(nullptr), type_enumerate(nullptr)
    {
    }

    void AstClass::set_type_enumerate(const neoast::AstEnum* type_enumerate_)
    {
        assert(!type_enumerate);
        type_enumerate = type_enumerate_;
    }

    bool AstClass::has_member(const std::string &name) const
    {
        return members.find(name) != members.end();
    }

    const Type* AstClass::get_member(const std::string &name) const
    {
        return members.at(name);
    }

    void AstClass::add_member(const std::string &name, Type* type, Compiler& c)
    {
        auto member_it = members.find(name);
        if (member_it == members.end())
        {
            // New member
            members.emplace(name, type);
        }
        else
        {
            // We overlap members here
            // We should find the greatest common divisor (via inheritance)
            Type* common_base = const_cast<Type*>(type->common_base(member_it->second));
            members[name] = common_base ? common_base : c.types.at(c.options["ast_class"]).get();
        }
    }

    void Type::resolve_inheritance(Compiler &c)
    {
    }

    void AstClass::resolve_inheritance(Compiler &c)
    {
        // Check if we are even inheriting from anyone
        if (decl.extends)
        {
            auto it = c.types.find(decl.extends->value);
            if (it == c.types.end())
            {
                c.context.emit_error(decl.extends.get(), "Failed to resolve parent class");
            }
            else
            {
                inherits_from = it->second.get();
            }
        }
    }

    const Type* AstClass::base_class() const
    {
        return inherits_from;
    }

    Type* AstClass::base_class()
    {
        return inherits_from;
    }

    std::string AstClass::get_definition(Compiler &c) const
    {
        std::stringstream ss;
        ss << "struct " << name().c_str() << " : public";

        ss << (inherits_from ? inherits_from->name() : c.options["ast_class"]);

        // Open brace
        ss << "\n{\n";

        if (decl.extra_code)
        {
            // Remove the braces
            // TODO(tumbar) Avoid placing braces in action code
            ss << decl.extra_code->value.substr(1, decl.extra_code->value.size() - 2);
        }

        for (const auto &member: members)
        {
            ss << variadic_string(
                    "    %s %s = %s;\n",
                    member_type().c_str(),
                    member.first.c_str(),
                    member_default().c_str());
        }

        ss << "\n};\n";
        return ss.str();
    }

    std::string AstClass::set_member(Compiler &c, const std::string &var_name, const std::string &member_name, const std::string &value) const
    {
        std::stringstream ss;
        std::string member_ref = var_name + "->" + member_name;

        auto* member = get_member(member_name);
        if (member)
        {
            ss << member->move_into(c, member_ref, value);
        }
        else
        {
            ss << member_ref << " = " << value;
        }

        return ss.str();
    }

    std::string AstClass::name() const
    {
        return decl.name->value;
    }

    std::string AstUniquePtrBase::union_type() const
    {
        return name() + "*";
    }

    std::string AstUniquePtrBase::member_type() const
    {
        return variadic_string("std::unique_ptr<%s>", name().c_str());
    }

    std::string AstUniquePtrBase::member_default() const
    {
        return "nullptr";
    }

    std::string AstUniquePtrBase::get_construction(Compiler &c, const std::string &var_name) const
    {
        return variadic_string("%s = new %s;",
                               var_name.c_str(),
                               name().c_str());
    }

    std::string AstUniquePtrBase::get_union_delete(Compiler &c, const std::string &var_name) const
    {
        return variadic_string("delete %s;", var_name.c_str());
    }

    std::string AstUniquePtrBase::move_into(Compiler &c, const std::string& destination, const std::string &value) const
    {
        return variadic_string(
                "%s = std::unique_ptr<%s>(%s)",
                destination.c_str(),
                name().c_str(),
                value.c_str());
    }

    std::string ManualDeclaration::get_definition(Compiler &c) const
    {
        // No action
        // We are using an externally defined type
        return "";
    }

    std::string ManualDeclaration::get_construction(Compiler &c, const std::string &var_name) const
    {
        return variadic_string(
                "%s = %s;",
                var_name.c_str(),
                decl.c_default->value.c_str());
    }

    std::string ManualDeclaration::get_union_delete(Compiler &c, const std::string &var_name) const
    {
        if (decl.c_delete)
        {
            std::string value = decl.c_delete->value;

            // Replace all occurrences of '$$'
            for (auto pos = value.find("$$"); pos != std::string::npos; pos = value.find("$$"))
            {
                value.replace(pos, 2, var_name);
            }

            return value + ";";
        }

        return "";
    }

    ManualDeclaration::ManualDeclaration(const ast::ManualDeclaration &decl_)
            : decl(decl_)
    {
    }

    std::string ManualDeclaration::name() const
    {
        return decl.name->value;
    }

    std::string ManualDeclaration::member_default() const
    {
        return decl.c_default->value;
    }

    TokenType::TokenType(std::string name_)
            : m_name(std::move(name_))
    {
    }

    std::string TokenType::name() const
    {
        return m_name;
    }

    std::string TokenType::union_type() const
    {
        return m_name + '*';
    }

    std::string TokenType::member_type() const
    {
        return variadic_string("std::unique_ptr<%s>", m_name.c_str());
    }
    std::string TokenType::member_default() const
    {
        return "nullptr";
    }

    std::string TokenType::get_definition(Compiler &c) const
    {
        std::stringstream ss;
        ss << "struct " << name() << " : public " << c.options["ast_class"];
        ss << ", public TokenPosition\n"
              "{\n"
              "    std::string value;\n"
              "\n"
              "    "
           << name()
           << "(const TokenPosition* position_, std::string value_)\n"
              "            : value(std::move(value_)), TokenPosition(*position_)\n"
              "    {\n"
              "    }\n"
              "};";

        return ss.str();
    }

    std::string TokenType::set_member(Compiler &c, const std::string &var_name, const std::string &member_name, const std::string &value) const
    {
        throw Exception("Token type does not support member setting");
    }

    std::string Type::get_union_delete(Compiler &c, const std::string &var_name) const
    {
        return "";
    }

    std::string Type::set_member(Compiler &c, const std::string &var_name, const std::string &member_name, const std::string &value) const
    {
        throw Exception("Set member is not support for this type");
    }

    std::string Type::move_into(Compiler &c, const std::string& destination, const std::string &value) const
    {
        return variadic_string(
                "%s = %s;",
                destination.c_str(),
                value.c_str());
    }

    const Type* Type::common_base(const Type* other) const
    {
        for (const Type* a = this; a != nullptr; a = a->base_class())
        {
            for (const Type* b = other; b != nullptr; b = b->base_class())
            {
                if (a == b) return a;
            }
        }

        return nullptr;
    }

    Type* Type::common_base(const Type* other)
    {
        for (Type* a = this; a != nullptr; a = a->base_class())
        {
            for (const Type* b = other; b != nullptr; b = b->base_class())
            {
                if (a == b) return a;
            }
        }

        return nullptr;
    }

    const Type* Type::base_class() const
    {
        return nullptr;
    }

    Type* Type::base_class()
    {
        return nullptr;
    }

    std::string AstType::get_definition(Compiler &c) const
    {
        const char* name = c.options["ast_class"].c_str();
        return variadic_string(R"(struct %s
{
    virtual ~%s() = default;
protected:
    // Force Ast to be abstract without pure virtual methods
    %s() = default;
};)", name, name, name);
    }

    std::string AstType::set_member(Compiler &c, const std::string &var_name, const std::string &member_name, const std::string &value) const
    {
        throw Exception("Cannot set member of base AstType");
    }

    AstType::AstType(std::string name_)
    : m_name(std::move(name_))
    {
    }

    std::string AstType::name() const
    {
        return m_name;
    }

    AstEnum::AstEnum(ast::Token& name_)
            : Positioned(name_), decl(name_)
    {
        add_enumerate(decl.value + "_NONE");
    }

    std::string AstEnum::get_definition(Compiler &c) const
    {
        std::stringstream ss;
        ss << "enum " << name() << "\n{\n";
        ss << "    " << name() << "_NONE,\n";

        for(const auto& item : enumerations)
        {
            ss << "    " << item << ",\n";
        }

        ss << "}\n";
        return ss.str();
    }

    std::string AstEnum::get_construction(Compiler &c, const std::string &var_name) const
    {
        const char* name = decl.value.c_str();
        return variadic_string("%s = %s_NONE;", var_name.c_str(), name);
    }

    std::string AstEnum::get_union_delete(Compiler &c, const std::string &var_name) const
    {
        return "";
    }

    void AstEnum::add_enumerate(const std::string& enumerate_name)
    {
        enumerations.emplace(enumerate_name);
    }

    std::string AstEnum::name() const
    {
        return decl.value;
    }

    std::string AstEnum::member_default() const
    {
        return name() + "_NONE";
    }

    AstVector::AstVector(ast::Token name_token_,
                         Type* element_type_)
            : Positioned(name_token_),
              m_name_token(std::move(name_token_)),
              m_element_type(element_type_)
    {
    }

    std::string AstVector::get_definition(Compiler &c) const
    {
        return variadic_string(
                "struct Vector_%s : public Ast\n"
                "{\n"
                "    %s all;\n"
                "};",
                m_name_token.value.c_str(),
                member_type().c_str());
    }

    void AstVector::resolve_inheritance(Compiler &c)
    {
        Type::resolve_inheritance(c);

        auto iter = c.types.find(m_name_token.value);
        if (iter == c.types.end())
        {
            c.context.emit_error(
                    &m_name_token,
                    "Undeclared element '%s' type for vector",
                    m_name_token.value.c_str());
        }
        else
        {
            m_element_type = iter->second.get();
        }
    }

    Type* AstVector::element_type() const
    {
        return m_element_type;
    }

    std::string AstVector::name() const
    {
        return "Vector_" + m_name_token.value;
    }

    std::string AstVector::member_type() const
    {
        return variadic_string("std::list<%s>",
                               m_element_type->member_type().c_str());
    }

    std::string AstVector::member_default() const
    {
        return "{}";
    }

    std::string AstVector::get_construction(Compiler &c, const std::string &var_name) const
    {
        return "new " + name();
    }

    std::string AstVector::get_union_delete(Compiler &c, const std::string &var_name) const
    {
        return "delete " + var_name + ';';
    }

    std::string AstVector::move_into(Compiler &c, const std::string &destination, const std::string &value) const
    {
        return variadic_string(
                "%s.splice(%s.begin(), %s->all); delete %s;",
                destination.c_str(),
                destination.c_str(),
                value.c_str(),
                value.c_str());
    }
}
