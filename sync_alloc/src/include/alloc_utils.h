//
// Created by SyncShard on 10/26/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_UTILS_H
#define ARENA_ALLOCATOR_ALLOC_UTILS_H

#include "defs.h"
#include "structs.h"
#include "types.h"

[[gnu::visibility("hidden")]]
extern _Thread_local arena_t *arena_thread;

[[gnu::visibility("hidden")]]
extern int bad_alloc_check(const syn_handle_t *restrict hdl, int do_checksum);

[[gnu::visibility("hidden"), maybe_unused]]
inline static int return_table_array(handle_table_t **arr) {
	if (arena_thread == nullptr || arena_thread->table_count == 0 || arena_thread->first_hdl_tbl == nullptr || arr == nullptr) {
		return 0;
	}
	handle_table_t *tbl = arena_thread->first_hdl_tbl;

	int idx = 0;
	while (idx < arena_thread->pool_count && tbl != nullptr) {
		arr[idx++] = tbl;
		tbl = tbl->next_table;
	}
	return idx;
}

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
[[gnu::visibility("hidden"), maybe_unused]]
inline static int return_pool_array(memory_pool_t **arr) {
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || arr == nullptr) {
		return 0;
	}
	memory_pool_t *pool = arena_thread->first_mempool;

	int idx = 0;
	for (; idx < arena_thread->pool_count; idx++) {
		arr[idx] = pool;
		pool = pool->next_pool;
	}
	return idx;
}

/**
 * Instead of walking the free list, this fills a VLA ptr array.
 * The array is not allocated, it has to be allocated before this function is called.
 *
 * @param arr The stack-allocated VLA to fill.
 * @param pool Which pool to walk the free list with.
 * @return How many free headers were found, equates directly to max index.
 *
 * @warning If any parameter is NULL or there is no list, this will return zero,
 * and not mutate the ptrs provided.
 */
[[gnu::visibility("hidden"), maybe_unused]]
inline static int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool) {
	if (arena_thread == nullptr || pool == nullptr || pool->first_free == nullptr || arr == nullptr) {
		return 0;
	}
	pool_free_node_t *head = pool->first_free;

	int idx = 0;
	while (idx < pool->free_count && head != nullptr) {
		arr[idx++] = head;
		head = head->next_free;
	}
	return idx;
}

/**
 * Checks for corruption in a given header.
 *
 * @param head The header to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
[[maybe_unused]]
inline static bool corrupt_header_check(pool_header_t *restrict head) {
	return (*(u32 *)((char *)head + (head->chunk_size - DEADZONE_PADDING)) != HEAD_DEADZONE);
}

/**
 * Checks for corruption in a given pool.
 *
 * @param pool The pool to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
[[maybe_unused]]
inline static bool corrupt_pool_check(memory_pool_t *pool) {
	return (*(u64 *)((char *)pool + PD_POOL_SIZE) != POOL_DEADZONE);
}

/**
 * Clears up defragmentation of the memory pool where there is any.
 *
 * @details Indexes through the memory pool from first to last,
 * if there are two adjacent blocks that are free, links the first free
 * to the next non-free, then updates head->chunk_size of the first free.
 * @param light_flag The light defrag option. If true, only a light defragmentation
 * will occur. If false, heavy defragmentation will occur.
 */
[[gnu::visibility("hidden")]]
extern void defragment_pool(bool light_flag);


/**
 * Handles the sentinel flags of the current and surrounding headers.
 *
 * @param head The header to index.
 */
[[gnu::visibility("hidden")]]
extern void update_sentinel_and_free_flags(pool_header_t *head);


/**
 * Handle generation checksum.
 *
 * @param hdl the handle to checksum.
 * @return returns true if checksum with the handle and entry is true, and false if not.
 */
[[gnu::visibility("hidden")]]
extern bool handle_generation_checksum(const syn_handle_t *restrict hdl);


/**
 * Updates the entry's generation, usually after a free.
 *
 * @param hdl The handle to update the table generation.
 */
[[gnu::visibility("hidden")]]
extern void update_table_generation(const syn_handle_t *restrict hdl);
[[gnu::visibility("hidden")]]
extern void table_destructor();
[[gnu::visibility("hidden")]]
extern void pool_destructor();

#endif //ARENA_ALLOCATOR_ALLOC_UTILS_H
