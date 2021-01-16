//
// Created by tumbar on 12/24/20.
//

#include <assert.h>
#include "clr_lalr.h"
#include "canonical_collection.h"

static inline
int lr_1_cmp_look_ahead(
        const U8 a[],
        const U8 b[],
        U32 tok_n)
{
    for (U32 i = 0; i < tok_n; i++)
    {
        if (a[i] != b[i])
            return 1;
    }

    return 0;
}

static inline
int lalr_1_cmp_prv(const LR_1* a, const LR_1* b, U32 tok_n)
{
    (void)tok_n;

    // Check that the grammars match
    // Check we are at the same position
    return a->grammar != b->grammar
           || (int)a->item_i - (int)b->item_i;
}

static inline
int clr_1_cmp_prv(const LR_1* a, const LR_1* b, U32 tok_n)
{
    // LALR(1) but we treat different lookaheads as different
    return lalr_1_cmp_prv(a, b, tok_n)
           || lr_1_cmp_look_ahead(a->look_ahead, b->look_ahead, tok_n);
}


int lalr_1_cmp(const GrammarState* a, const GrammarState* b, U32 tok_n)
{
    // Make sure all items in a exist in b
    int a_item_n = 0;
    int b_item_n = 0;
    for (const LR_1* iter = a->head_item; iter; a_item_n++, iter = iter->next);
    for (const LR_1* iter = b->head_item; iter; b_item_n++, iter = iter->next);

    if (a_item_n != b_item_n)
    {
        return a_item_n - b_item_n;
    }

    for (LR_1* a_c = a->head_item; a_c; a_c = a_c->next)
    {
        U8 found_match = 0;
        for (const LR_1* b_c = b->head_item; b_c; b_c = b_c->next)
        {
            if (lalr_1_cmp_prv(a_c, b_c, tok_n) == 0)
            {
                found_match = 1;
                break;
            }
        }

        if (!found_match)
        {
            return 1;
        }
    }

    return 0;
}

int clr_1_cmp(const GrammarState* a, const GrammarState* b, U32 tok_n)
{
    // Make sure all items in a exist in b
    int a_item_n = 0;
    int b_item_n = 0;
    for (const LR_1* iter = a->head_item; iter; a_item_n++, iter = iter->next);
    for (const LR_1* iter = b->head_item; iter; b_item_n++, iter = iter->next);

    if (a_item_n != b_item_n)
    {
        return a_item_n - b_item_n;
    }

    for (LR_1* a_c = a->head_item; a_c; a_c = a_c->next)
    {
        U8 found_match = 0;
        for (const LR_1* b_c = b->head_item; b_c; b_c = b_c->next)
        {
            if (clr_1_cmp_prv(a_c, b_c, tok_n) == 0)
            {
                found_match = 1;
                break;
            }
        }

        if (!found_match)
        {
            return 1;
        }
    }

    return 0;
}

void lalr_1_merge(GrammarState* target, const GrammarState* to_merge, U32 tok_n)
{
    // Merge the lookaheads
    LR_1* iter = target->head_item;

    for (; iter; iter = iter->next)
    {
        LR_1* iter_merge = to_merge->head_item;
        assert(iter_merge && "Empty LALR(1) state");

        // Find the correct LR_1 item to merge with
        while (iter_merge && lalr_1_cmp_prv(iter_merge, iter, tok_n))
        {
            iter_merge = iter_merge->next;
        }

        assert(iter_merge && "LR(1) item comparison failure during merge");
        lookahead_merge(iter->look_ahead, iter_merge->look_ahead, tok_n);
    }
}
