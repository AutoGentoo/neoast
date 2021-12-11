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

#ifndef NEOAST_CG_PATTERN_H
#define NEOAST_CG_PATTERN_H

#include <ostream>
#include <reflex/pattern.h>

/**
 * Generate the FSM code to implement the regular
 * expression from the DFA.
 * @param os output stream to dump to
 * @param pat pattern to generate FSM for
 * @param func_name function name where to place the FSM code
 */
void cg_pattern_put_fsm(
        std::ostream &os,
        const std::string &regex,
        const std::string& func_name);

#endif //NEOAST_CG_PATTERN_H
