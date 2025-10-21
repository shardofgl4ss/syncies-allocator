//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "../../include/structs.h"

/**	@brief Creates a new block header
 *
 *	@details Flag options: 0 = ALLOCATED, 1 = FREE, 2 = FROZEN
 *
 *	@param pool Pointer to the pool to create the header in.
 *	@param size Size of the block and header.
 *	@param offset How many bytes from the base of the pool.
 *	@return a valid block header pointer.
 *
 *	@note Does not update memory offset of the pool.
 *	@note The header is set as ALLOCATED by default.
 */
static Pool_Header *
mempool_create_header(const Memory_Pool *restrict pool, u16 size, u32 offset);

/** @brief Analyzes the header-block chain, making a new header if there is no free one to use.
 *	If there is no space at all in the pool, it will create a new pool for it.
 *	@param arena The given arena to find a block in.
 *	@param requested_size User-requested size, aligned by ALIGNMENT.
 *	@return header of a valid block, if creating one was successful.
 *
 *	@warning Will return NULL if a new pool's header could not be made, which should be impossible.
 */
static Pool_Header *
mempool_find_block(const Arena *restrict arena, u16 requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H