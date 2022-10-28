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


#include <yacc_codegen/regex.h>

extern "C" {
    #include <cmocka.h>
}

#define CTEST(name) static void name(void** state)

CTEST(test_macro_expansion_1)
{
    MacroEngine m_engine;
    m_engine.add("identifier", "[A-z_][A-z_0-9]*");

    auto expanded = m_engine.expand("{identifier}(\\/{identifier})?");
    assert_string_equal(expanded.c_str(), "[A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?");
}

CTEST(test_macro_expansion_2)
{
    MacroEngine m_engine;
    m_engine.add("identifier", "[A-z_][A-z_0-9]*");
    m_engine.add("atom", "{identifier}(\\/{identifier})?");
    m_engine.add("version", "(-[0-9]+[a-z]?)+");
    m_engine.add("atom_with_version", "{atom}{version}");

    auto expanded = m_engine.expand("emerge -qUDN {atom_with_version}::gentoo");
    auto expanded_2 = m_engine.expand("{atom}");
    assert_string_equal(expanded.c_str(), "emerge -qUDN [A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?(-[0-9]+[a-z]?)+::gentoo");
    assert_string_equal(expanded_2.c_str(), "[A-z_][A-z_0-9]*(\\/[A-z_][A-z_0-9]*)?");
}


const static struct CMUnitTest macro_tests[] = {
        cmocka_unit_test(test_macro_expansion_1),
        cmocka_unit_test(test_macro_expansion_2),
};

int main()
{
    return cmocka_run_group_tests(macro_tests, nullptr, nullptr);
}
