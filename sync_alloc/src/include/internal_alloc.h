//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "structs.h"


/**	@brief Creates a new block header
 *
 *	@param pool Pointer to the pool to create the header in.
 *	@param size Size of the block and header.
 *	@param offset How many bytes from the base of the pool.
 *	@return a valid block header pointer.
 *
 *	@warning Updates offset in the pool if successful.
 *	@note The header is set as ALLOCATED by default.
 */
ATTR_PRIVATE extern Pool_Header *
mempool_create_header(Memory_Pool *restrict pool, u32 size, intptr offset);


/** @brief Analyzes the header-block chain, making a new header if there is no free one to use.
 *	If there is no space at all in the pool, it will create a new pool for it.
 *	@param requested_size User-requested size, aligned by ALIGNMENT.
 *	@return header of a valid block, if creating one was successful.
 *
 *	@warning Will return NULL if a new pool's header could not be made, which should be impossible.
 *	It will only happen if all pools cannot fit the allocation.
 */
ATTR_PRIVATE extern Pool_Header *
mempool_find_block(u32 requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H