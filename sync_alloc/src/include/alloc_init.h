//
// Created by SyncShard on 10/16/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_INIT_H
#define ARENA_ALLOCATOR_ALLOC_INIT_H

#include "debug.h"
#include "structs.h"


/// @brief Creates a new arena in thread-local storage. Each thread must create its own arena.
/// @return 0 on success, -1 on failure.
///
/// @warning The arena ptr in TLS will be a nullptr if there is not enough memory.
/// If this happens, the library will terminate.
ATTR_PRIVATE extern int
arena_init();

/// @brief Creates a new memory pool.
///	@param size How many bytes to give to the new pool.
///	@return	Returns a pointer to the new memory pool.
///
///	@note Handles linking pools, and updating the total_mem_size in the arena struct.
///	@warning Returns NULL if allocating a new pool fails, or if provided size is zero.
ATTR_PRIVATE extern Memory_Pool *
pool_init(u32 size);

ATTR_PRIVATE _Noreturn extern void
arena_panic();

#endif //ARENA_ALLOCATOR_ALLOC_INIT_H