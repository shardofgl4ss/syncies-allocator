//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_LIB_H
#define ARENA_ALLOCATOR_ALLOC_LIB_H


#include "defs.h"
#include "structs.h"
#include "helper_functions.h"
#include "internal_alloc.h"


Arena *
arena_create();

void
arena_destroy(Arena *restrict arena);

static void
arena_defragment(Arena *arena, Defrag_Type defrag);

Arena_Handle
arena_alloc(Arena *arena, size_t size);

void
arena_reset(const Arena *restrict arena, int reset_type);

void
arena_free(Arena_Handle *user_handle);

void *
arena_lock(Arena_Handle *user_handle);

void
arena_debug_print_memory_usage(const Arena *arena);

#endif //ARENA_ALLOCATOR_ALLOC_LIB_H