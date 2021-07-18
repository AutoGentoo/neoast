//
// Created by tumbar on 7/17/21.
//

#ifndef NEOAST_CG_GRAMMAR_H
#define NEOAST_CG_GRAMMAR_H

#include <codegen/codegen.h>
#include <codegen/codegen_priv.h>

struct CGGrammarsImpl;

class CGGrammars
{
    CGGrammarsImpl* impl_;
public:
    CGGrammars(const CodeGen* cg, const File* self);

    void put_table(std::ostream &os) const;
    void put_rules(std::ostream &os) const;
    void put_actions(std::ostream &os) const;

    uint32_t size() const;
    GrammarRule* get() const;

    ~CGGrammars();
};

#endif //NEOAST_CG_GRAMMAR_H
