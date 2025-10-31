//
// Created by SyncShard on 10/26/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_UTILS_H
#define ARENA_ALLOCATOR_ALLOC_UTILS_H

#include "structs.h"
#include "types.h"

[[gnu::visibility("hidden")]]
extern _Thread_local Arena *arena_thread;

[[gnu::visibility("hidden")]]
extern int bad_alloc_check(struct Arena_Handle *restrict hdl);

[[gnu::visibility("hidden")]]
inline static int return_table_array(Handle_Table **arr) {
	if (arena_thread == nullptr || arena_thread->table_count == 0 || arena_thread->first_hdl_tbl == nullptr ||
	    arr == nullptr) {
		return 0;
	}
	Handle_Table *tbl = arena_thread->first_hdl_tbl;

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
[[gnu::visibility("hidden")]]
inline static int return_pool_array(Memory_Pool **arr) {
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || arr == nullptr) {
		return 0;
	}
	Memory_Pool *pool = arena_thread->first_mempool;

	int idx = 0;
	while (idx < arena_thread->pool_count && pool != nullptr) {
		arr[idx++] = pool;
		pool = pool->next_pool;
	}
	return idx;
}

/**
 * Instead of walking the free list, this fills a VLA ptr array.
 * The array is not allocated, it has to be allocated before this function is called.
 *
 * @param arr The stack-allocated VLA to fill.
 * @param pool Which pool to walk the free list with..
 * @return How many free headers were found, equates directly to max index.
 *
 * @warning If any parameter is NULL or there is no list, this will return zero,
 * and not mutate the ptrs provided.
 */
[[gnu::visibility("hidden")]]
inline static int return_free_array(Pool_Free_Header **arr, const Memory_Pool *pool) {
	if (arena_thread == nullptr || pool == nullptr || pool->first_free == nullptr || arr == nullptr) {
		return 0;
	}
	Pool_Free_Header *head = pool->first_free;

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
inline static bool corrupt_header_check(Pool_Header *restrict head) {
	return (*(u64 *)((char *)head + (head->size - DEADZONE_PADDING)) != POOL_DEADZONE);
}

/**
 * Checks for corruption in a given pool.
 *
 * @param pool The pool to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
[[maybe_unused]]
inline static bool corrupt_pool_check(Memory_Pool *pool) {
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
ATTR_PRIVATE extern void defragment_pool(bool light_flag);


/**
 * Handles the sentinel flags of the current and surrounding headers.
 *
 * @param head The header to index.
 */
ATTR_PRIVATE extern void update_sentinel_flags(Pool_Header *head);


/**
 * Handle generation checksum.
 *
 * @param hdl the handle to checksum.
 * @return returns true if checksum with the handle and entry is true, and false if not.
 */
ATTR_PRIVATE extern bool handle_generation_checksum(const struct Arena_Handle *restrict hdl);


/**
 * Updated the entry's generation, usually after a free.
 *
 * @param hdl The handle to update the table generation.
 */
ATTR_PRIVATE extern void update_table_generation(const struct Arena_Handle *restrict hdl);

ATTR_PRIVATE extern void table_destructor();

ATTR_PRIVATE extern void pool_destructor();

#endif //ARENA_ALLOCATOR_ALLOC_UTILS_H
