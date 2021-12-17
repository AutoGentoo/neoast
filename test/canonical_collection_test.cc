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

enum
{
    TOK_EOF = NEOAST_ASCII_MAX,
    TOK_a,
    TOK_b,
    TOK_S,
    TOK_A,
    TOK_AUGMENT
};

static uint32_t r1[] = {
        TOK_A,
        TOK_A
};

static uint32_t r2[] = {
        TOK_a,
        TOK_A
};

static uint32_t r3[] = {
        TOK_b
};

static uint32_t a_r[] = {
        TOK_AUGMENT
};

static GrammarRule g_rules[] = {
        {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r},
        {.token = TOK_S, .tok_n = 2, .grammar = r1},
        {.token = TOK_A, .tok_n = 2, .grammar = r2},
        {.token = TOK_A, .tok_n = 1, .grammar = r3},
};

static const char* token_error_names[] = {
        "EOF",
        "a",
        "b",
        "S",
        "A",
        "augment"
};

#define LR_S(i) (((uint32_t)(i)) | TOK_SHIFT_MASK)
#define LR_R(i) (((uint32_t)(i)) | TOK_REDUCE_MASK)
#define LR_E() TOK_SYNTAX_ERROR
#define LR_A() TOK_ACCEPT_MASK

static const
uint32_t expected_lalr1_table[] = {
        LR_E( ), LR_S(3), LR_S(4), LR_S(1), LR_S(2), /* 0 */
        LR_A( ), LR_A( ), LR_A( ), LR_E( ), LR_E( ), /* 1 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(5), /* 2 */
        LR_E( ), LR_S(3), LR_S(4), LR_E( ), LR_S(6), /* 3 */
        LR_R(3), LR_R(3), LR_R(3), LR_E( ), LR_E( ), /* 4 */
        LR_R(1), LR_E( ), LR_E( ), LR_E( ), LR_E( ), /* 5 */
        LR_R(2), LR_R(2), LR_R(2), LR_E( ), LR_E( ), /* 6 */
};


static GrammarParser p;
void initialize_parser()
{
    static const uint32_t r1[] = {TOK_A, TOK_A};
    static const uint32_t r2[] = {TOK_a, TOK_A};
    static const uint32_t r3[] = {TOK_b};
    static const uint32_t a_r[] = {TOK_S};
    static const GrammarRule g_rules[] = {
            {.token = TOK_AUGMENT, .tok_n = 1, .grammar = a_r},
            {.token = TOK_S, .tok_n = 2, .grammar = r1},
            {.token = TOK_A, .tok_n = 2, .grammar = r2},
            {.token = TOK_A, .tok_n = 1, .grammar = r3},
    };

    p.grammar_n = 4;
    p.grammar_rules = g_rules;
    p.token_n = TOK_AUGMENT - NEOAST_ASCII_MAX;
    p.action_token_n = 3;
    p.token_names = token_error_names;
    p.parser_reduce = (parser_reduce) nullptr;
}

CTEST(test_state_hash)
{
    std::unordered_set<std::shared_ptr<GrammarState>,
                GrammarState::Hasher, GrammarState::Equal> test_set;

    BitVector lookaheads(3);
    lookaheads.select(0);

    std::vector<LR1> items1{
            {&g_rules[0], 0, lookaheads},
            {&g_rules[1], 0, lookaheads},
    };

    std::vector<LR1> items2{
            {&g_rules[1], 0, lookaheads},
            {&g_rules[0], 0, lookaheads},
    };

    std::vector<LR1> items3{
            {&g_rules[1], 0, lookaheads},
            {&g_rules[0], 1, lookaheads},
    };

    std::shared_ptr<GrammarState> s1(new GrammarState(nullptr, items1));
    std::shared_ptr<GrammarState> s2(new GrammarState(nullptr, items2));
    std::shared_ptr<GrammarState> s3(new GrammarState(nullptr, items3));

    assert_int_equal(s1->hash(), s2->hash());

    test_set.insert(s1);
    assert_true(test_set.find(s2) != test_set.end());

    assert_int_not_equal(s1->hash(), s3->hash());
    assert_true(test_set.find(s3) == test_set.end());
}

CTEST(test_tablegen)
{
    CanonicalCollection cc(&p, nullptr);
    cc.resolve();

    uint8_t error;
    uint32_t* table = cc.generate(nullptr, &error);
    assert_int_equal(error, 0);

    dump_table(table, &cc, "$abSAP", 0, stdout, nullptr);
    fprintf(stdout, "===========\n");
    dump_table(expected_clr1_lalr1_table, &cc, "$abSAP", 0, stdout, nullptr);
    assert_memory_equal(table, expected_clr1_lalr1_table, sizeof(expected_clr1_lalr1_table));

    free(table);
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
            cmocka_unit_test(test_bit_vector),
            cmocka_unit_test(test_tablegen),
    };

    initialize_parser();

    return cmocka_run_group_tests(cc_tests, nullptr, nullptr);
}
