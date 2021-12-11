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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lexer/container.h"

void neoast_vector_init(NeoastVector* self, size_t item_len)
{
    self->n = 0;
    self->s = 16;
    self->item_len = item_len;

    self->ptr = malloc(item_len * self->s);
}

void neoast_vector_resize(NeoastVector* self, size_t next_size, int fill)
{
    if (next_size > self->n)
    {
        // Make sure there is enough space
        neoast_vector_grow(self, next_size);
        memset((char*)self->ptr + (self->n * self->item_len), fill, self->item_len * (next_size - self->n));
    }
    self->n = next_size;
}

void neoast_vector_grow(NeoastVector* self, size_t next_size)
{
    if (next_size > self->s)
    {
        self->s = next_size;
        self->ptr = realloc(self->ptr, sizeof(self->item_len) * self->s);
    }
}

void neoast_vector_push_back(NeoastVector* self, void* item)
{
    if (self->n + 1 >= self->s)
    {
        // Double the size
        neoast_vector_grow(self, self->s << 1);
    }

    // Add the item to the vector
    memcpy((char*)self->ptr + (self->n++ * self->item_len), item, self->item_len);
}

void neoast_vector_push_back_i(NeoastVector* self, int item)
{
    neoast_vector_push_back(self, &item);
}

void neoast_vector_push_back_s(NeoastVector* self, size_t item)
{
    neoast_vector_push_back(self, &item);
}

void neoast_vector_free(NeoastVector* self)
{
    if (self->ptr) free(self->ptr);
    self->ptr = NULL;
}

void neoast_stack_pop(NeoastStack* self, void* dest)
{
    assert(self->n > 0);
    memcpy(dest, &self->ptr[--self->n], self->item_len);
}
