//
// Created by tumbar on 1/15/21.
//

#include <stdio.h>
#include <cmocka.h>

#define CTEST(name) static void name(void** state)

// Pretend header
void* calc_init();
void* calc_allocate_buffers();
void calc_free_buffers(void* self);
double calc_parse(const void* self, const void* buffers, const char* input);

CTEST(test_parser)
{
    void* parser = calc_init();
    void* buffers = calc_allocate_buffers();

    const char* input = "3 + 5 + (4 * 2 + (5 / 2))";
    double result = calc_parse(parser, buffers, input);
    calc_free_buffers(buffers);
    assert_double_equal(result, 3 + 5 + (4 * 2 + (5.0 / 2)), 0.001);
}

const static struct CMUnitTest left_scan_tests[] = {
        cmocka_unit_test(test_parser),
};

int main()
{
    return cmocka_run_group_tests(left_scan_tests, NULL, NULL);
}