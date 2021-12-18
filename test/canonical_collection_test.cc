/*
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <neoast.h>
#include <parsergen/canonical_collection.h>
#include <util/util.h>

extern "C" {
#include <stdio.h>
#include <cmocka.h>
}

#define CTEST(name) extern "C" void name(void** state)

using namespace parsergen;

namespace simple
{
    enum
    {
        TOK_EOF = NEOAST_ASCII_MAX,
        TOK_a,
        TOK_b,
        TOK_S,
        TOK_A,
        TOK_AUGMENT
    };


    static const uint32_t simple_rules[][2] = {
            {TOK_S},
            {TOK_A, TOK_A},
            {TOK_a, TOK_A},
            {TOK_b}
    };

    static const GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = simple_rules[0]},
            {.token = TOK_S, .tok_n = 2, .grammar = simple_rules[1]},
            {.token = TOK_A, .tok_n = 2, .grammar = simple_rules[2]},
            {.token = TOK_A, .tok_n = 1, .grammar = simple_rules[3]},
    };

    static const char* token_names[] = {
            "$",
            "a",
            "b",
            "S",
            "A",
            "P"
    };
}

#define LR_S(i) (((uint32_t)(i)) | TOK_SHIFT_MASK)
#define LR_R(i) (((uint32_t)(i)) | TOK_REDUCE_MASK)
#define LR_E() TOK_SYNTAX_ERROR
#define LR_A() TOK_ACCEPT_MASK

static const
uint32_t expected_lalr1_table[] = {
        LR_E( ), LR_S(3), LR_S(4), LR_S(1), LR_S(2), /* 0 */
        LR_A( ), LR_E( ), LR_E( ), LR_E( ), LR_E( ), /* 1 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(6), /* 2 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(5), /* 3 */
        LR_R(3), LR_R(3), LR_R(3), LR_E( ), LR_E( ), /* 4 */
        LR_R(2), LR_R(2), LR_R(2), LR_E( ), LR_E( ), /* 5 */
        LR_R(1), LR_E( ), LR_E( ), LR_E( ), LR_E( ), /* 6 */
};

void initialize_parser(GrammarParser& p,
                       uint32_t augment,
                       const GrammarRule g_rules[],
                       const char* token_names[])
{
    p.grammar_n = 4;
    p.grammar_rules = g_rules;
    p.token_n = augment - NEOAST_ASCII_MAX;
    p.action_token_n = 3;
    p.token_names = token_names;
    p.parser_reduce = (parser_reduce) nullptr;
}

static GrammarParser simple_p;
static GrammarParser bootstrap_p;

CTEST(test_state_hash)
{
    std::unordered_set<sp<GrammarState>,
                HasherPtr<sp<GrammarState>>,
                EqualizerPtr<sp<GrammarState>>> test_set;

    BitVector lookaheads(3);
    lookaheads.select(0);

    std::vector<LR1> items1{
            {&simple::g_rules[0], 0, lookaheads},
            {&simple::g_rules[1], 0, lookaheads},
    };

    std::vector<LR1> items2{
            {&simple::g_rules[1], 0, lookaheads},
            {&simple::g_rules[0], 0, lookaheads},
    };

    std::vector<LR1> items3{
            {&simple::g_rules[1], 0, lookaheads},
            {&simple::g_rules[0], 1, lookaheads},
    };

    sp<GrammarState> s1(new GrammarState(nullptr, items1));
    sp<GrammarState> s2(new GrammarState(nullptr, items2));
    sp<GrammarState> s3(new GrammarState(nullptr, items3));

    assert_int_equal(s1->hash(), s2->hash());

    auto p = test_set.insert(s1);
    assert_true(p.second);
    assert_true(test_set.find(s2) != test_set.end());

    assert_int_not_equal(s1->hash(), s3->hash());
    assert_true(test_set.find(s3) == test_set.end());
}

CTEST(test_lr1_lr0_sorting)
{
    BitVector l1(3);
    l1.select(0);

    BitVector l2(3);
    l1.select(0);
    l1.select(1);

    LR1 i1(&simple::g_rules[1], 0, l1);
    LR1 i2(&simple::g_rules[1], 0, l2);
    LR1 i3(&simple::g_rules[1], 1, l1);

    // Different lookaheads should not change the hash
    assert_int_equal(i1.hash(), i2.hash());

    // Different item will change the hash
    assert_int_not_equal(i1.hash(), i3.hash());

    std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> items;

    auto p = items.emplace(&simple::g_rules[1], 0, l1);
    assert_true(p.second);
    p = items.emplace(&simple::g_rules[1], 0, l1);
    assert_false(p.second);
    p = items.emplace(&simple::g_rules[1], 0, l2);
    assert_true(p.second);
}

CTEST(test_tablegen)
{
    CanonicalCollection cc(&simple_p, nullptr);
    cc.resolve(LALR_1);

    uint8_t error;
    uint32_t* table = cc.generate(nullptr, &error);
    assert_int_equal(error, 0);

    dump_table(table, &cc, simple::token_names, 0, stdout, nullptr);
    fprintf(stdout, "===========\n");
    dump_table(expected_lalr1_table, &cc, simple::token_names, 0, stdout, nullptr);
    assert_memory_equal(table, expected_lalr1_table, sizeof(expected_lalr1_table));

    free(table);
}

CTEST(test_lookaheads)
{
    CanonicalCollection cc(&simple_p, nullptr);

    BitVector first_of_A(simple_p.action_token_n);
    first_of_A.select(1);
    first_of_A.select(2);

    BitVector lookaheads(simple_p.action_token_n);
    bool has_empty;
    cc.merge_first_of(lookaheads, simple::TOK_A, has_empty);

    assert_false(has_empty);
    assert_true(lookaheads == first_of_A);
}

CTEST(test_lalr1_consolidation)
{
    std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> lr1_1;
    std::unordered_set<LR1, Hasher<LR1>, Equalizer<LR1>> lr1_2;

    BitVector l1(3);
    l1.select(0);

    BitVector l2(3);
    l1.select(0);
    l1.select(1);

    auto p = lr1_1.emplace(&simple::g_rules[1], 0, l1);
    assert_true(p.second); // make sure it was added
    p = lr1_1.emplace(&simple::g_rules[2], 0, l1);
    assert_true(p.second);

    p = lr1_2.emplace(&simple::g_rules[1], 0, l2);
    assert_true(p.second);
    p = lr1_2.emplace(&simple::g_rules[2], 0, l1);
    assert_true(p.second);

    // Under CLR(1) table, these are distinct states
    assert_false(lr1_1 == lr1_2);

    // Under LALR(1) table, these are identical states
    assert_true(lalr_equal(lr1_1, lr1_1));
}

CTEST(test_bit_vector)
{
    BitVector v(10);
    v.select(2);
    v.select(4);
    v.select(7);
    v.select(9);

    auto i = v.begin();
    assert_int_equal(*i, 2);
    ++i; assert_int_equal(*i, 4);
    ++i; assert_int_equal(*i, 7);
    ++i; assert_int_equal(*i, 9);
    ++i; assert_false(i != v.end());

    assert_true(v.has_any());
    assert_false(v.has_none());

    v.clear();
    assert_true(v.has_none());
}

int main()
{
    const static struct CMUnitTest cc_tests[] = {
            cmocka_unit_test(test_state_hash),
            cmocka_unit_test(test_lr1_lr0_sorting),
            cmocka_unit_test(test_bit_vector),
            cmocka_unit_test(test_tablegen),
            cmocka_unit_test(test_lookaheads),
            cmocka_unit_test(test_lalr1_consolidation),
    };

    initialize_parser(simple_p, simple::TOK_AUGMENT, simple::g_rules, simple::token_names);

    return cmocka_run_group_tests(cc_tests, nullptr, nullptr);
}
