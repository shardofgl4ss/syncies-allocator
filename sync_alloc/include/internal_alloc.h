//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "helper_functions.h"

static Arena_Handle
mempool_create_handle_and_entry(Arena *restrict arena, Pool_Header *restrict head);

/** @brief Creates a new handle table. Does not increment table_count by itself, do that before calling.
 *
 * @param arena The arena to create a new table on.
 * @param table The table to map.
 * @return a valid handle table if there is enough system memory.
 */
static Handle_Table *
mempool_new_handle_table(Arena *restrict arena, Handle_Table *restrict table);

/**	@brief Creates a new block header
 *
 *	@details Flag options: 0 = ALLOCATED, 1 = FREE, 2 = FROZEN
 *
 *	@param pool Pointer to the pool to create the header in.
 *	@param size Size of the block and header.
 *	@param offset How many bytes from the base of the pool.
 *	@param pool_id The ID of the pool the header is in, to detect cross-pool linkage. Starts from 0.
 *	@return a valid block header pointer.
 *
 *	@note Does not link headers. Does not update memory offset of the pool.
 *	@note The header is set as ALLOCATED by default.
 */
Pool_Header *
mempool_create_header(const Memory_Pool *restrict pool, u16 size, usize offset);

/** @brief Creates a new memory pool.
 *	@param arena Pointer to the arena to create a new pool in.
 *	@param size How many bytes to give to the new pool.
 *	@return	Returns a pointer to the new memory pool.
 *
 *	@note Handles linking pools, and updating the total_mem_size in the arena struct.
 *	@warning Returns NULL if allocating a new pool fails, or if provided size is zero.
 */
static Memory_Pool *
mempool_create_internal_pool(Arena *restrict arena, size_t size);

/** @brief Analyzes the header-block chain, making a new header if there is no free one to use.
 *	If there is no space at all in the pool, it will create a new pool for it.
 *	@param arena The given arena to find a block in.
 *	@param requested_size User-requested size, aligned by ALIGNMENT.
 *	@return header of a valid block, if creating one was successful.
 *
 *	@warning Will return NULL if a new pool's header could not be made, which should be impossible.
 */
static Pool_Header *
mempool_find_block(Arena *arena, size_t requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H