//
// Created by SyncShard on 11/15/25.
//

#ifndef ARENA_ALLOCATOR_FREE_NODE_H
#define ARENA_ALLOCATOR_FREE_NODE_H

#include "types.h"

typedef struct Memory_Pool memory_pool_t;


/**
 * 	Freed memory pool block header. in a singly-linked-list style.
 *
 * 	@details
 * 	Casting a header to and from pool_free_header_t and pool_header_t,
 * 	will preserve size and bitflags, but not preserve pool_free_node_t's
 * 	next_free or pool_header_t's handle_idx.
 *
 *	@details
 *	see Pool_Header for more details.
 */
typedef struct Pool_Free_Node {
	struct Pool_Free_Node *next_free;
	u32 chunk_size;
	bit32 bitflags;
} __attribute__((aligned(16))) pool_free_node_t;


/**
 *	Instead of walking the free list, this fills a VLA ptr array.
 *	The array is not allocated, it has to be allocated before this function is called.
 *
 *	@param arr The stack-allocated VLA to fill.
 *	@param pool Which pool to walk the free list with.
 *	@return How many free headers were found, equates directly to max index.
 *
 *	@warning If any parameter is NULL or there is no list, this will return zero,
 *	and not mutate the ptrs provided.
 */
extern int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool);

extern int free_node_add(memory_pool_t *pool, pool_free_node_t *free_node);
//int free_node_remove(memory_pool_t *pool, pool_free_node_t *free_node);

#endif //ARENA_ALLOCATOR_FREE_NODE_H