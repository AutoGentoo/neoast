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


#include <string.h>
#include <codegen/regex.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cmocka.h>

#define CTEST(name) static void name(void** state)

CTEST(test_macro_expansion_1)
{
    MacroEngine* m_engine = macro_engine_init();
    macro_engine_register(m_engine, "identifier", "[A-z_][A-z_0-9]*");

    char* expanded = regex_expand(m_engine, "{identifier}(\\/{identifier})?");
    assert_string_equal(expanded, "[A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?");

    free(expanded);
    macro_engine_free(m_engine);
}

CTEST(test_macro_expansion_2)
{
    MacroEngine* m_engine = macro_engine_init();
    macro_engine_register(m_engine, "identifier", "[A-z_][A-z_0-9]*");
    macro_engine_register(m_engine, "atom", "{identifier}(\\/{identifier})?");
    macro_engine_register(m_engine, "version", "(-[0-9]+[a-z]?)+");
    macro_engine_register(m_engine, "atom_with_version", "{atom}{version}");

    char* expanded = regex_expand(m_engine, "emerge -qUDN {atom_with_version}::gentoo");
    char* expanded_2 = regex_expand(m_engine, "{atom}");
    assert_string_equal(expanded, "emerge -qUDN [A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?(-[0-9]+[a-z]?)+::gentoo");
    assert_string_equal(expanded_2, "[A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?");

    free(expanded);
    free(expanded_2);
    macro_engine_free(m_engine);
}


const static struct CMUnitTest macro_tests[] = {
        cmocka_unit_test(test_macro_expansion_1),
        cmocka_unit_test(test_macro_expansion_2),
};

int main()
{
    return cmocka_run_group_tests(macro_tests, NULL, NULL);
}
