//
// Created by tumbar on 6/20/21.
//

#ifndef NEOAST_CODEGEN_PRIV_H
#define NEOAST_CODEGEN_PRIV_H


#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <reflex.h>
#include <reflex/matcher.h>
#include "cg_util.h"
#include "regex.h"


#define CODEGEN_STRUCT "NeoastValue"
#define CODEGEN_UNION "NeoastUnion"


struct CGToken;
struct CGGrammarToken;
struct CGTyped;
struct CGAction;
struct CodeGenImpl;


extern const TokenPosition NO_POSITION;
extern std::string grammar_filename;
template<typename T> using up = std::unique_ptr<T>;
template<typename T> using sp = std::shared_ptr<T>;

class CodeGen
{
    CodeGenImpl* impl_;

public:
    explicit CodeGen(const File* self);

    sp<CGToken> get_token(const std::string &name) const;
    const char* get_start_token() const;
    const std::vector<std::string>& get_tokens() const;
    const Options& get_options() const;

    void write_header(std::ostream &os) const;
    void write_source(std::ostream &os) const;

    ~CodeGen();
};

struct Code : public TokenPosition
{
    std::string code;   ///< line of code
    std::string file;   ///< source filename

    Code(const char* value, const TokenPosition* position) :
            code(value), file(grammar_filename), TokenPosition(*position) {}

    explicit Code(const KeyVal* self)
            : code(self->value),
              file(grammar_filename),
              TokenPosition(self->position)
    {
    }

    bool empty() const { return code.empty(); }
    std::string get_simple() const;
    std::string get_complex(
            const Options& options,
            const std::vector<std::string>& argument_replace,
            const std::string& zero_arg,
            const std::string& non_zero_arg,
            bool is_union = false) const;

private:
    static bool in_redzone(const std::vector<std::pair<int, int>>& redzones, const reflex::AbstractMatcher& m)
    {
        for (const auto& iter : redzones)
        {
            if (iter.first > m.first() && iter.second < m.last())
            {
                return true;
            }

            // Future redzones are always outside the bounds of this match
            if (iter.first > m.last())
            {
                return false;
            }
        }

        return false;
    }
};

struct CGToken : public TokenPosition
{
    std::string name;
    int id;

    CGToken(const TokenPosition* position,
            std::string name,
            int id)
            : TokenPosition(position ? *position : NO_POSITION),
            name(std::move(name)), id(id) {}

    // Make all children virtual so that we have access to dynamic casting
    virtual ~CGToken() = default;
};

struct CGTyped : public CGToken
{
    std::string type;           //!< Field in union

    CGTyped(const KeyVal* self, int id)
            : CGToken(&self->position, self->value, id), type(self->key) {}

    CGTyped(const TokenPosition* position,
            const std::string &type,
            const std::string &name,
            int id) : CGToken(position, name, id), type(type) {}
};

struct CGGrammarToken : public CGTyped
{
    CGGrammarToken(const KeyVal* self, int id) : CGTyped(self, id) {}
    CGGrammarToken(const TokenPosition* position,
                   const std::string &type,
                   const std::string &name,
                   int id) : CGTyped(position, type, name, id) {}
};

struct CGAction : public CGToken
{
    bool is_ascii;

    CGAction(const KeyVal* self, int id, bool is_ascii = false)
            : CGToken(&self->position, self->value, id), is_ascii(is_ascii) {}

    CGAction(const TokenPosition* position, std::string name,
             int id, bool is_ascii = false)
            : CGToken(position, std::move(name), id), is_ascii(is_ascii) {}
};

struct CGTypedAction : public CGTyped, public CGAction
{
    CGTypedAction(const KeyVal* self, int id) : CGTyped(self, id), CGAction(self, id) {}
};

#endif //NEOAST_CODEGEN_PRIV_H
