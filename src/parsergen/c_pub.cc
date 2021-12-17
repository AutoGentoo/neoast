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

#include "c_pub.h"
#include "canonical_collection.h"

void* canonical_collection_init(const GrammarParser* parser, const void* debug_info)
{
    return new parsergen::CanonicalCollection(parser, static_cast<const parsergen::DebugInfo*>(debug_info));
}

void canonical_collection_resolve(void* self, parser_t p_type)
{
    static_cast<parsergen::CanonicalCollection*>(self)->resolve();
}

uint32_t* canonical_collection_generate(const void* self, const uint8_t* precedence_table, uint8_t* error)
{
    return static_cast<const parsergen::CanonicalCollection*>(self)->generate(precedence_table, error);
}

void canonical_collection_free(void* self)
{
    delete static_cast<const parsergen::CanonicalCollection*>(self);
}
