//
// Created by SyncShard on 10/26/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_UTILS_H
#define ARENA_ALLOCATOR_ALLOC_UTILS_H

#include "defs.h"
#include "structs.h"
#include "types.h"
#include <sys/mman.h>

[[gnu::visibility("hidden")]]
extern _Thread_local arena_t *arena_thread;

[[gnu::visibility("hidden")]]
extern int bad_alloc_check(const syn_handle_t *restrict hdl, int do_checksum);


/// @brief Destroys a heap allocation. Just a wrapper for mmap() to reduce includes.
/// @param mem The heap to destroy.
/// @param bytes The size of the heap.
/// @return 0 if successful, -1 for errors.
extern int syn_unmap_page(void *restrict mem, usize bytes);


/// @brief Allocates memory via mmap(). Each map is marked NORESERVE and ANONYMOUS.
/// Just a wrapper for mmap() to reduce includes.
/// @param bytes How many bytes to allocate.
/// @return voidptr to the heap region.
[[nodiscard, gnu::malloc(munmap, 1), gnu::alloc_size(1)]]
extern void *syn_map_page(usize bytes);


[[gnu::visibility("hidden"), maybe_unused]]
extern int return_table_array(handle_table_t * *arr);

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
extern int return_pool_array(memory_pool_t * *arr);


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
[[gnu::visibility("hidden")]]
extern int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool);

/**
 * Checks for corruption in a given header.
 *
 * @param head The header to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
[[maybe_unused]]
inline static bool corrupt_header_check(pool_header_t * restrict head)
{
	return (*(u32 *)((char *)head + (head->chunk_size - DEADZONE_PADDING)) != HEAD_DEADZONE);
}


/**
 * Checks for corruption in a given pool.
 *
 * @param pool The pool to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
[[maybe_unused]]
inline static bool corrupt_pool_check(memory_pool_t *pool)
{
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
extern void update_sentinel_and_free_flags(pool_header_t * head);


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