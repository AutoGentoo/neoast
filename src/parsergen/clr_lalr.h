//
// Created by tumbar on 12/24/20.
//

#ifndef NEOAST_CLR_LALR_H
#define NEOAST_CLR_LALR_H

#include <common.h>

int lalr_1_cmp(const GrammarState* a, const GrammarState* b, U32 tok_n);
void lalr_1_merge(GrammarState* target, const GrammarState* to_merge, U32 tok_n);
int clr_1_cmp(const GrammarState* a, const GrammarState* b, U32 tok_n);

#endif //NEOAST_CLR_LALR_H