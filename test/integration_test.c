//
// Created by tumbar on 1/15/21.
//

#include <stdio.h>
#include <cmocka.h>

#define CTEST(name) static void name(void** state)

// Pretend header
uint32_t calc_init();
void* calc_allocate_buffers();
void calc_free_buffers(void* self);
void calc_free();
double calc_parse(const void* buffers, const char* input);

uint32_t calc_ascii_init();
void* calc_ascii_allocate_buffers();
void calc_ascii_free_buffers(void* self);
void calc_ascii_free();
double calc_ascii_parse(const void* buffers, const char* input);

CTEST(test_empty)
{
    assert_int_equal(calc_init(), 0);
    void* buffers = calc_allocate_buffers();

    assert_double_equal(calc_parse(buffers, "   "), 0, 0);

    calc_free();
    calc_free_buffers(buffers);
}

CTEST(test_parser)
{
    assert_int_equal(calc_init(), 0);
    void* buffers = calc_allocate_buffers();

    const char* input = "3 + 5 + (4 * 2 + (5 / 2))";
    double result = calc_parse(buffers, input);
    assert_double_equal(result, 3 + 5 + (4 * 2 + (5.0 / 2)), 0.001);
    calc_free_buffers(buffers);
    calc_free();
}

CTEST(test_empty_ascii)
{
    assert_int_equal(calc_ascii_init(), 0);
    void* buffers = calc_ascii_allocate_buffers();

    assert_double_equal(calc_ascii_parse(buffers, "   "), 0, 0);

    calc_ascii_free();
    calc_ascii_free_buffers(buffers);
}

CTEST(test_parser_ascii)
{
    assert_int_equal(calc_ascii_init(), 0);
    void* buffers = calc_ascii_allocate_buffers();

    const char* input = "3 + 5 + (4 * 2 + (5 / 2))";
    double result = calc_ascii_parse(buffers, input);
    assert_double_equal(result, 3 + 5 + (4 * 2 + (5.0 / 2)), 0.001);
    calc_ascii_free_buffers(buffers);
    calc_ascii_free();
}

const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_parser),
        cmocka_unit_test(test_empty_ascii),
        cmocka_unit_test(test_parser_ascii),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}