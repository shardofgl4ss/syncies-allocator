//
// Created by SyncShard on 10/26/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_UTILS_H
#define ARENA_ALLOCATOR_ALLOC_UTILS_H

#include "structs.h"
#include "sync_alloc.h"
#include <stdint.h>

extern _Thread_local arena_t *arena_thread;


[[maybe_unused]]
#ifdef SYN_USE_RAW
static inline void *invalid_block()
{
	return nullptr;
}

extern int bad_alloc_check(const void *, const int do_checksum);
#else
static inline syn_handle_t invalid_block()
{
	const syn_handle_t hdl = {
		.generation = UINT32_MAX,
		.addr = nullptr,
		.header = nullptr,
	};
	return hdl;
}


extern int bad_alloc_check(const syn_handle_t *restrict hdl, int do_checksum);

[[gnu::pure]]
extern memory_pool_t *return_pool(const pool_header_t *restrict header);

#endif

extern pool_header_t *return_header(void *block_ptr);


/**
 * Instead of walking the linked list of pools, this fills a VLA ptr array.
 * The array is not allocated, it has to be allocated before this function is called.
 *
 * @param arr The stack-allocated VLA to fill.
 * @return How many pools were found, equates directly to max index.
 *
 * @warning If any parameter is NULL or there is no list, this will return zero,
 * and not mutate the ptrs provided.
 */
[[maybe_unused]]
extern int return_pool_array(memory_pool_t **arr);

/**
 * Clears up defragmentation of the memory pool where there is any.
 *
 * @details Indexes through the memory pool from first to last,
 * if there are two adjacent blocks that are free, links the first free
 * to the next non-free, then updates head->chunk_size of the first free.
 * will occur. If false, heavy defragmentation will occur.
 */
extern void defragment_pool();


/**
 * Handles the sentinel flags of the current and surrounding headers.
 *
 * @param head The header to index.
 */
extern void update_sentinel_and_free_flags(pool_header_t *head);


extern void pool_destructor();

#endif //ARENA_ALLOCATOR_ALLOC_UTILS_H