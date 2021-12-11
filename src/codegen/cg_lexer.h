#ifndef NEOAST_CG_LEXER_H
#define NEOAST_CG_LEXER_H

#include <iostream>
#include <reflex/pattern.h>

class CGLexer
{
    /**
     * Subclasses must implement the global and init
     * functions to paste into the codegen
     */
public:
    virtual void put_top(std::ostream &os) const = 0;
    virtual void put_global(std::ostream &os) const = 0;
    virtual void put_bottom(std::ostream &os) const = 0;

    virtual std::string get_init() const = 0;
    virtual std::string get_delete() const = 0;

    /**
     * Create and destroy parsing instances of the lexer
     */
    virtual std::string get_new_inst(const std::string &name) const = 0;
    virtual std::string get_del_inst(const std::string &name) const = 0;
    virtual std::string get_ll_next(const std::string &name) const = 0;
};

#endif //NEOAST_CG_LEXER_H
