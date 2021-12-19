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

#include "regex.h"
#include <reflex/matcher.h>
#include <reflex/pattern.h>

struct MacroEngineImpl
{
    std::unordered_map<std::string, std::string> map;
    std::map<std::string, std::string> original_map;
    reflex::Pattern macro_pattern;

    MacroEngineImpl() : macro_pattern("\\{[A-Za-z_][A-Za-z0-9_]*\\}") {}
};

MacroEngine::MacroEngine()
: impl_(new MacroEngineImpl)
{
}

MacroEngine::~MacroEngine()
{
    delete impl_;
}

std::string MacroEngine::expand(const std::string &regex) const
{
    reflex::Matcher m(impl_->macro_pattern, regex);
    std::ostringstream ss;

    size_t last_col = 0;
    for (auto& match : m.find)
    {
        ss << regex.substr(last_col, match.first() - last_col);
        last_col = match.last();

        std::string macro_name(match.text() + 1);
        macro_name.pop_back(); // remove '}'

        if (impl_->map.find(macro_name) != impl_->map.end())
        {
            ss << impl_->map.at(macro_name);
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

void MacroEngine::add(const std::string &name, const std::string &value)
{
    impl_->map[name] = expand(value);
    impl_->original_map[name] = value;
}

const std::map<std::string, std::string> &MacroEngine::get_original() const
{
    return impl_->original_map;
}
