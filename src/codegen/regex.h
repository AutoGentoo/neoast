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


#ifndef NEOAST_REGEX_H
#define NEOAST_REGEX_H

#include <string>
#include <unordered_map>
#include <map>
#include <sstream>

struct MacroEngineImpl;
class MacroEngine
{
    MacroEngineImpl* impl_;

public:
    MacroEngine();
    ~MacroEngine();

    std::string expand(const std::string& regex) const;
    void add(const std::string& name, const std::string& value);
    const std::map<std::string, std::string>& get_original() const;
};

#endif //NEOAST_REGEX_H
