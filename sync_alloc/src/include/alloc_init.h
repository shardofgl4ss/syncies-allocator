//
// Created by SyncShard on 10/16/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_INIT_H
#define ARENA_ALLOCATOR_ALLOC_INIT_H

#include "debug.h"
#include "structs.h"

ATTR_PRIVATE extern Arena *
arena_init();

/** @brief Creates a new memory pool.
 *	@param arena Pointer to the arena to create a new pool in.
 *	@param size How many bytes to give to the new pool.
 *	@return	Returns a pointer to the new memory pool.
 *
 *	@note Handles linking pools, and updating the total_mem_size in the arena struct.
 *	@warning Returns NULL if allocating a new pool fails, or if provided size is zero.
 */
ATTR_PRIVATE extern Memory_Pool *
pool_init(Arena *arena, u32 size);


#endif //ARENA_ALLOCATOR_ALLOC_INIT_H