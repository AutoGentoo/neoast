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

#include <reflex/pattern.h>
#include <string>
#include <unordered_map>
#include <reflex/matcher.h>
#include <sstream>

class MacroEngine
{
    std::unordered_map<std::string, std::string> map;
    std::map<std::string, std::string> original_map;
    reflex::Pattern macro_pattern;

public:
    MacroEngine() : macro_pattern("\\{[A-Za-z_][A-Za-z0-9_]*\\}")
    {
    }

    std::string expand(const std::string& regex) const
    {
        reflex::Matcher m(macro_pattern, regex);
        std::ostringstream ss;

        size_t last_col = 0;
        for (auto& match : m.find)
        {
            ss << regex.substr(last_col, match.first() - last_col);
            last_col = match.last();

            std::string macro_name(match.text() + 1);
            macro_name.pop_back(); // remove '}'

            if (map.find(macro_name) != map.end())
            {
                ss << map.at(macro_name);
            }
            else
            {
                // Don't try to expand this
                std::cerr << "Invalid macro name " << macro_name << "\n";
            }
        }

        // Place any remaining characters
        if (last_col < regex.size())
        {
            ss << regex.substr(last_col);
        }
        return ss.str();
    }

    void add(const std::string& name, const std::string& value)
    {
        map[name] = expand(value);
        original_map[name] = value;
    }

    const std::map<std::string, std::string>& get_original() const { return original_map; }
};

#endif //NEOAST_REGEX_H
