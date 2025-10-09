//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "helper_functions.h"

static Arena_Handle
mempool_create_handle_and_entry(const Arena *restrict arena, Mempool_Header *restrict head);

static Handle_Table *
mempool_new_handle_table(Arena *restrict arena);

Mempool_Header *
mempool_create_header(const Mempool *pool, size_t size, size_t offset, Header_Block_Flags flag);

static Mempool *
mempool_create_internal_pool(Arena *restrict arena, size_t size);

static Mempool_Header *
mempool_find_block(Arena *arena, size_t requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H