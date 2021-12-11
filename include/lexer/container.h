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

#ifndef NEOAST_CONTAINER_H
#define NEOAST_CONTAINER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_AT(v, i, t) *((t*) ((char*)(v).ptr + ((i) * (v).item_len)))
#define VECTOR_BACK(v, t) *((t*) ((char*)(v).ptr + (((v).n - 1) * (v).item_len)))

struct NeoastVector_prv {
    void* ptr;
    size_t n;
    size_t s;
    size_t item_len;
};

// NeoastVector API
void vector_init(NeoastVector* self, size_t item_len);
void vector_push_back(NeoastVector* self, void* item);
void vector_resize(NeoastVector* self, size_t next_size, int fill);
void vector_grow(NeoastVector* self, size_t next_size);
void vector_push_back_i(NeoastVector* self, int item);
void vector_push_back_s(NeoastVector* self, size_t item);
void vector_free(NeoastVector* self);

// NeoastStack API
#define stack_init vector_init
#define stack_push vector_push_back
#define stack_push_i vector_push_back_i
#define stack_free vector_free
void stack_pop(NeoastStack* self, void* dest);
#define stack_peek(s, t) VECTOR_BACK(s, t)

#ifdef __cplusplus
};
#endif

#endif //NEOAST_CONTAINER_H
