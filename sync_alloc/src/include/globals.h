//
// Created by SyncShard on 11/17/25.
//

#ifndef ARENA_ALLOCATOR_GLOBALS_H
#define ARENA_ALLOCATOR_GLOBALS_H

#include "defs.h"
#include "structs.h"
#include "types.h"

constexpr u32 KIBIBYTE = 1024;
constexpr u32 MEBIBYTE = 1024 * KIBIBYTE;
constexpr u32 GIBIBYTE = 1024 * MEBIBYTE;
constexpr u32 MINIMUM_BLOCK_ALLOC = 64;
constexpr u32 MAX_ALLOC_HUGE_SIZE = GIBIBYTE * 4;
constexpr u32 MAX_ALLOC_POOL_SIZE = KIBIBYTE * 128;
constexpr u32 MAX_FIRST_POOL_SIZE = KIBIBYTE * 128;
constexpr u32 MAX_POOL_SIZE = GIBIBYTE * 2;
constexpr u32 MAX_TABLE_HNDL_COLS = 64;
constexpr u32 MAX_ALLOC_SLAB_SIZE = 256;
constexpr u32 STRUCT_SIZE_ARENA = sizeof(arena_t);
constexpr u32 STRUCT_SIZE_POOL = sizeof(memory_pool_t);
constexpr u32 STRUCT_SIZE_HEADER = sizeof(pool_header_t);
constexpr u32 STRUCT_SIZE_FREE_NODE = sizeof(pool_free_node_t);
constexpr u32 DEADZONE_SIZE = sizeof(pool_deadzone_t);
constexpr u32 RESERVED_FIRST_POOL_SIZE =
	((STRUCT_SIZE_ARENA + STRUCT_SIZE_POOL) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 DEADZONE_PADDING = sizeof(u64);
constexpr u32 HEAD_DEADZONE = 0xDEADDEADU;
constexpr u64 POOL_DEADZONE = 0xDEADDEADDEADDEADULL;

static_assert(sizeof(pool_deadzone_t) == sizeof(head_deadzone_t),
              "error: deadzone sizes do not match!\n");

#endif //ARENA_ALLOCATOR_GLOBALS_H
