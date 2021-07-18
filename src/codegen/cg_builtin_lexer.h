#ifndef NEOAST_CG_BUILTIN_LEXER_H
#define NEOAST_CG_BUILTIN_LEXER_H

#include <codegen/codegen.h>
#include <codegen/codegen_priv.h>
#include <codegen/cg_lexer.h>

struct CGBuiltinLexerImpl;

class CGBuiltinLexer : public CGLexer
{
    CGBuiltinLexerImpl* impl_;

public:
    explicit CGBuiltinLexer(const File* self, MacroEngine &m_engine_,
                            const Options &options);

    void put_header(std::ostream &os) const override;
    void put_global(std::ostream &os) const override;
    const Options& get_options() const;
    std::string get_init() const override;
    std::string get_delete() const override;
    ~CGBuiltinLexer();

    std::string get_new_inst(const std::string &name) const override;
    std::string get_del_inst(const std::string& name) const override;
    std::string get_ll_next(const std::string& name) const override;
};

#endif //NEOAST_CG_BUILTIN_LEXER_H
