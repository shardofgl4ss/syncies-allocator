//
// Created by SyncShard on 10/16/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_INIT_H
#define ARENA_ALLOCATOR_ALLOC_INIT_H

#include "debug.h"
#include "structs.h"

static Arena *
arena_init();

/** @brief Creates a new memory pool.
 *	@param arena Pointer to the arena to create a new pool in.
 *	@param size How many bytes to give to the new pool.
 *	@return	Returns a pointer to the new memory pool.
 *
 *	@note Handles linking pools, and updating the total_mem_size in the arena struct.
 *	@warning Returns NULL if allocating a new pool fails, or if provided size is zero.
 */
ATTR_ALLOC_SIZE(2) static Memory_Pool *
pool_init(Arena *arena, u32 size);

inline static void
debug_init()
{
	logger.debug_print_all = &debug_print_memory_usage;
	logger.to_console = &log_to_console;
}

#endif //ARENA_ALLOCATOR_ALLOC_INIT_H