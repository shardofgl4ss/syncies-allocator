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
constexpr u32 PD_ARENA_SIZE = (sizeof(arena_t) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 PD_POOL_SIZE = (sizeof(memory_pool_t) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 PD_HEAD_SIZE = (sizeof(pool_header_t) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 PD_FREE_PH_SIZE = (sizeof(pool_free_node_t) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 PD_RESERVED_F_SIZE = ((PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
constexpr u32 DEADZONE_PADDING = sizeof(u64);
constexpr u32 HEAD_DEADZONE = 0xDEADDEADU;
constexpr u64 POOL_DEADZONE = 0xDEADDEADDEADDEADULL;

#endif //ARENA_ALLOCATOR_GLOBALS_H