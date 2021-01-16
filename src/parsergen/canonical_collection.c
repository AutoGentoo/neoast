//
// Created by tumbar on 12/24/20.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "canonical_collection.h"
#include "parser.h"
#include "clr_lalr.h"

static inline
U32 token_to_index(U32 token, const GrammarParser* parser)
{
    if (token < ASCII_MAX)
    {
        if (parser->ascii_mappings)
        {
            fprintf(stderr, "ASCII characters are not valid grammar tokens\n");
            abort();
        }
    }
    else if (parser->ascii_mappings)
    {
        return token - ASCII_MAX;
    }

    return token;
}

static inline
void lr_1_free(LR_1* self)
{
    LR_1* next;
    while (self)
    {
        next = self->next;
        free(self);
        self = next;
    }
}

static inline
void gs_free(GrammarState* self)
{
    lr_1_free(self->head_item);
    free(self);
}

CanonicalCollection* canonical_collection_init(const GrammarParser* parser)
{
    CanonicalCollection* self = malloc(sizeof(CanonicalCollection));
    self->parser = parser;
    self->dfa = NULL;
    self->state_n = 0;
    self->state_s = 32;
    self->all_states = malloc(sizeof(GrammarState*) * self->state_s);
    return self;
}

static GrammarState* gs_init(const CanonicalCollection* parent)
{
    return calloc(1,
                  sizeof(GrammarState)
                  + sizeof (GrammarState*) * parent->parser->token_n);
}

static LR_1* lr_1_init(const GrammarRule* rule, U32 item_i, U32 token_n)
{
    LR_1* self = malloc(sizeof(LR_1) + sizeof(U8) * token_n);
    self->next = NULL;
    self->grammar = rule;
    self->item_i = item_i;
    self->final_item = self->item_i >= rule->tok_n;
    memset(self->look_ahead, 0, token_n);

    return self;
}

static void
lookahead_copy(U8 dest[], const U8 src[], U32 n)
{
    memcpy(dest, src, sizeof(U8) * n);
}

void lookahead_merge(U8 dest[], const U8 src[], U32 n)
{
    for (U32 i = 0; i < n; i++)
        dest[i] = dest[i] || src[i];
}


U8 lr_1_firstof(U8 dest[], U32 token, const GrammarParser* parser)
{
    if (token < parser->action_token_n)
    {
        // Action tokens cannot be expanded
        dest[token] = 1;
        return 0;
    }

    U8 merge_current_lookahead = 0;

    // This rule can be expanded
    // Recursively get the first of this token
    for (U32 i = 0; i < parser->grammar_n; i++)
    {
        const GrammarRule* rule = &parser->grammar_rules[i];
        if (rule->tok_n == 0)
        {
            // Empty rule
            merge_current_lookahead = 1;
            continue;
        }

        // Recursive grammar rules
        // We can skip this one
        if (rule->grammar[0] == token)
        {
            continue;
        }

        if (rule->token == token)
        {
            merge_current_lookahead =
                    lr_1_firstof(dest, rule->grammar[0], parser)
                    || merge_current_lookahead;
        }
    }

    return merge_current_lookahead;
}

static void gs_apply_closure(GrammarState* self, const GrammarParser* parser)
{
    // Find all rules in this state that
    // are currently looking at tokens generated
    // from other grammar rules
    // Expand those token's rules

    U8* already_expanded = alloca(sizeof(U8) * (parser->token_n - parser->action_token_n));
    memset(already_expanded, 0, sizeof(U8) * (parser->token_n - parser->action_token_n));
    U8 dirty = 1;

    while (dirty) // Recursively generate closure rules
    {
        dirty = 0;
        for (LR_1* item = self->head_item; item; item = item->next)
        {
            if (item->final_item)
                continue;

            // Find if this rule can be expanded
            U32 potential_token = item->grammar->grammar[item->item_i];
            if (token_to_index(potential_token, parser) < parser->action_token_n)
            {
                // Action tokens cannot be expanded
                continue;
            }

            // Generate the lookahead for this rule
            U8* temp_lookahead = alloca(sizeof(U8) * parser->action_token_n);
            memset(temp_lookahead, 0, sizeof(U8) * parser->action_token_n);

            if (item->final_item || item->item_i + 1 >= item->grammar->tok_n)
            {
                // This is the last item in the list
                // The next item is our own look ahead
                lookahead_copy(temp_lookahead, item->look_ahead, parser->action_token_n);
            }
            else
            {
                // The look ahead is the next item in our grammar
                U32 lookahead_token_idx = token_to_index(item->grammar->grammar[item->item_i + 1], parser);
                if (lookahead_token_idx < parser->action_token_n)
                {
                    temp_lookahead[lookahead_token_idx] = 1;
                }
                else
                {
                    U8 merge_our_lookahead = lr_1_firstof(
                            temp_lookahead, item->grammar->grammar[item->item_i + 1],
                            parser);

                    if (merge_our_lookahead)
                    {
                        lookahead_merge(temp_lookahead, item->look_ahead, parser->action_token_n);
                    }
                }
            }

            if (already_expanded[token_to_index(potential_token, parser) - parser->action_token_n])
            {
                // This is already expanded
                // We need to merge the lookahead with every other
                // rule that matches this token
                for (LR_1* item_iter_2 = self->head_item; item_iter_2; item_iter_2 = item_iter_2->next)
                {
                    if (item_iter_2->grammar->token == potential_token)
                    {
                        lookahead_merge(item_iter_2->look_ahead, temp_lookahead, parser->action_token_n);
                    }
                }
                continue;
            }

            dirty = 1;
            already_expanded[token_to_index(potential_token, parser) - parser->action_token_n] = 1;

            U32 expand_count = 0;
            for (U32 i = 0; i < parser->grammar_n; i++)
            {
                // Check if this token is the result of any grammar rule
                const GrammarRule* rule = &parser->grammar_rules[i];
                if (rule->token == potential_token)
                {
                    LR_1* new_rule = lr_1_init(rule, 0, parser->action_token_n);

                    new_rule->next = item->next;
                    item->next = new_rule;
                    lookahead_copy(new_rule->look_ahead, temp_lookahead, parser->action_token_n);
                    expand_count++;
                }
            }

            // Make sure there is at least one
            // rule describing this expression
            if (!expand_count)
            {
                fprintf(stderr, "Could not find a rule to describe token '%d'\n", potential_token);
                abort();
            }
        }
    }

    // We now have closure :)
}

static GrammarState* cc_generate_augmented(const CanonicalCollection* cc)
{
    GrammarState* initial = gs_init(cc);
    LR_1* augmented_item = lr_1_init(&cc->parser->grammar_rules[0],
                                     0, cc->parser->action_token_n);
    augmented_item->look_ahead[0] = 1;

    initial->head_item = augmented_item;
    gs_apply_closure(initial, cc->parser);
    return initial;
}

void canonical_collection_free(CanonicalCollection* self)
{
    for (U32 i = 0; i < self->state_n; i++)
    {
        gs_free(self->all_states[i]);
    }

    free(self->all_states);
    free(self);
}

static void
cc_add_state(CanonicalCollection* cc,
             GrammarState* s)
{
    if (cc->state_n + 1 >= cc->state_s)
    {
        cc->state_s *= 2;
        cc->all_states = realloc(cc->all_states, sizeof(GrammarState*) * cc->state_s);
    }

    cc->all_states[cc->state_n++] = s;
}

static
void gs_resolve(CanonicalCollection* cc, GrammarState* state)
{
    // Assume we already have closure
    for (const LR_1* item = state->head_item; item; item = item->next)
    {
        // Nothing left to do here
        if (item->final_item)
            continue;

        // Create the new item to hold
        // the state of this grammar rule
        LR_1* new_item = lr_1_init(item->grammar, item->item_i + 1, cc->parser->action_token_n);
        lookahead_copy(new_item->look_ahead, item->look_ahead, cc->parser->action_token_n);

        U32 next_token = token_to_index(item->grammar->grammar[item->item_i], cc->parser);
        if (state->action_states[next_token])
        {
            // Another rule already generated this state
            // Add this rule to the state
            // We don't need to do anything here
        }
        else
        {
            // We need to generate a new state for this
            // unhandled token
            state->action_states[next_token] = gs_init(cc);
        }

        // Add the lr_1 item to the target state
        new_item->next = state->action_states[next_token]->head_item;
        state->action_states[next_token]->head_item = new_item;
    }

    for (I32 token = cc->parser->token_n - 1; token >= 0; token--)
    {
        if (!state->action_states[token])
            continue;

        // We need to generate closures and resolve
        gs_apply_closure(state->action_states[token], cc->parser);

        // Check if this state matches any other added states
        U8 is_duplicate = 0;
        for (U32 i = 0; i < cc->state_n; i++)
        {
            if (clr_1_cmp(cc->all_states[i],
                      state->action_states[token],
                      cc->parser->action_token_n) == 0)
            {
                // These states match
                // We should free the duplicate state
                is_duplicate = 1;
                gs_free(state->action_states[token]);
                state->action_states[token] = cc->all_states[i];

                break;
            }
        }

        if (is_duplicate)
            continue;

        // We haven't seen this state yet
        // We need to register this state
        cc_add_state(cc, state->action_states[token]);
        gs_resolve(cc, state->action_states[token]);
    }
}

void canonical_collection_resolve(
        CanonicalCollection* self,
        parser_t p_type)
{
    // Generate the initial by create an
    // augmented state and applying the closure
    self->dfa = cc_generate_augmented(self);
    self->state_n = 0;
    self->all_states[self->state_n++] = self->dfa;

    // Resolve the DFA
    // Repeated states are consolidated -- CLR(1)
    gs_resolve(self, self->dfa);

    // LALR(1) tables do no distinguish states by different
    // lookaheads. We can't merge these states during resolution
    // because states are subject to change during resolution
    // This means we need to delay LALR(1) consolidation here.
    // As of right now, we have a valid CLR(1) DFA
    if (p_type == LALR_1)
    {
        // We need to consolidate states that have
        // the LR(1) items but different lookaheads
        for (U32 i = 0; i < self->state_n; i++)
        {
            for (U32 j = i + 1; j < self->state_n; j++)
            {
                // If states at i and j match, we need
                // to merge j into i.
                // After we merge j into i we need to
                // replace all uses of j with i
                // We will move the state in the final slot
                // into this slot to consolidate to array

                if (lalr_1_cmp(self->all_states[i],
                               self->all_states[j],
                               self->parser->token_n) == 0)
                {
                    // i and j match

                    lalr_1_merge(self->all_states[i], self->all_states[j], self->parser->token_n);

                    // Find all uses of j in all states
                    for (U32 k = 0; k < self->state_n; k++)
                    {
                        for (U32 action_i = 0; action_i < self->parser->token_n; action_i++)
                        {
                            if (self->all_states[k]->action_states[action_i] == self->all_states[j])
                            {
                                // State j is used here, replace it with i
                                self->all_states[k]->action_states[action_i] = self->all_states[i];
                            }
                        }
                    }

                    // Free j state
                    gs_free(self->all_states[j]);
                    self->all_states[j] = NULL;

                    // Make the original state array smaller
                    self->all_states[j] = self->all_states[--self->state_n];
                }
            }
        }
    }
}

static inline
U32 cc_get_grammar_id(const GrammarParser* self, const GrammarRule* rule)
{
    for (U32 i = 0; i < self->grammar_n; i++)
    {
        if (rule == &self->grammar_rules[i])
            return i;
    }

    fprintf(stderr, "Grammar not found\n");
    exit(1);
}

static inline
U32 cc_get_state_id(const CanonicalCollection* self, const GrammarState* state)
{
    for (U32 i = 0; i < self->state_n; i++)
    {
        if (state == self->all_states[i])
            return i;
    }

    fprintf(stderr, "State not found\n");
    exit(1);
}

static inline
U32* cc_allocate_table(const CanonicalCollection* self)
{
    return calloc(self->state_n * self->parser->token_n, sizeof(U32));
}

U32* canonical_collection_generate(const CanonicalCollection* self, const U8* precedence_table)
{
    U32 rr_conflicts = 0, sr_conflicts = 0;
    U32* table = cc_allocate_table(self); // zeroes

    U32 i;
    for (U32 state_i = 0; state_i < self->state_n; state_i++)
    {
        const GrammarState* state = self->all_states[state_i];
        i = state_i * self->parser->token_n;

        for (U32 tok_j = 0; tok_j < self->parser->token_n; tok_j++, i++)
        {
            // There are three options
            // 1. Syntax error
            // 2. Reduce (handled in another loop)
            // 3. Shift

            if (!state->action_states[tok_j])
            {
                // This is not a branch in the DFA
                // Syntax error
                table[i] = TOK_SYNTAX_ERROR;
            }
            else
            {
                U32 target_state = cc_get_state_id(
                        self, state->action_states[tok_j]);

                table[i] = target_state | TOK_SHIFT_MASK;
            }
        }

        // Check if there are any reduce moves
        // ... and fill in the table
        for (const LR_1* item = state->head_item;
             item; item = item->next)
        {
            if (item->final_item)
            {
                U32 action_mask;
                U32 grammar_id;
                if (token_to_index(item->grammar->token, self->parser) == self->parser->token_n) // Augmented rule
                {
                    // This is an accept
                    action_mask = TOK_ACCEPT_MASK;
                    grammar_id = 0;
                    assert(state->head_item == item
                           && !item->next
                           && "Accept state cannot have multiple rules");
                }
                else
                {
                    action_mask = TOK_REDUCE_MASK;
                    // This is a reduce expression
                    // Get the grammar rule we want to reduce
                    grammar_id = cc_get_grammar_id(self->parser, item->grammar);
                }

                // Only add reduce for the items in this lookahead
                for (U32 lookahead_i = 0; lookahead_i < self->parser->action_token_n; lookahead_i++)
                {
                    if (!item->look_ahead[lookahead_i])
                        continue; // Not a lookahead

                    U32 idx = lookahead_i + (state_i * self->parser->token_n);

                    if (table[idx])
                    {
                        // TODO Better conflict debugging
                        if (table[idx] & TOK_REDUCE_MASK)
                        {
                            fprintf(stderr, "RR conflict at state %d tok %d (R%d, R%d)\n",
                                    state_i, lookahead_i,
                                    grammar_id, table[idx] & TOK_MASK);
                            rr_conflicts++;
                        }
                        else if (table[idx] & TOK_SHIFT_MASK)
                        {
                            if (precedence_table && precedence_table[lookahead_i] == PRECEDENCE_LEFT)
                            {
                                // Let the table value take reduction
                            }
                            else if (precedence_table && precedence_table[lookahead_i] == PRECEDENCE_RIGHT)
                            {
                                // Keep the shift
                                continue;
                            }
                            else
                            {
                                fprintf(stderr, "SR conflict at state %d tok %d\n", state_i, lookahead_i);
                                sr_conflicts++;
                            }
                        }
                    }
                    table[idx] = action_mask | grammar_id;
                }
            }
        }
    }

    if (rr_conflicts)
    {
        fprintf(stderr, "Warning: %d RR conflicts\n", rr_conflicts);
    }
    if (sr_conflicts)
    {
        fprintf(stderr, "Warning: %d SR conflicts\n", sr_conflicts);
    }

    return table;
}
