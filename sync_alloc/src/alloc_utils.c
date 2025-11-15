//
// Created by SyncShard on 10/26/25.
//

#include "alloc_utils.h"
#include "alloc_init.h"
#include "debug.h"
#include "defs.h"
#include "handle.h"
#include "structs.h"
#include "types.h"
#include <asm-generic/mman-common.h>
#include <linux/mman.h>
#include <sys/mman.h>

// Not implemented
// void
// defragment_pool(Memory_Pool *pool);

#ifdef SYN_USE_RAW

inline int bad_alloc_check(const void *, const int do_checksum)
{
}

#else

inline int bad_alloc_check(const syn_handle_t *restrict hdl, const int do_checksum)
{
	if (arena_thread == nullptr) {
		syn_panic("core arena context was lost!\n");
	}
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (corrupt_header_check(hdl->header)) {
		syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	}
	if (do_checksum && !handle_generation_checksum(hdl)) {
		sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
		return 1;
	}
	if (hdl->header->bitflags & F_FROZEN) {
		return 2;
	}
	#endif
	return 0;
}


syn_handle_t *return_handle(const u32 encoded_matrix_index)
{
	const u32 row = encoded_matrix_index / MAX_TABLE_HNDL_COLS;
	const u32 col = encoded_matrix_index % MAX_TABLE_HNDL_COLS;

	handle_table_t *table = arena_thread->first_hdl_tbl;

	for (u32 i = 0; i < row; i++) {
		table = table->next_table;
	}

	return &table->handle_entries[col];
}


memory_pool_t *return_pool(pool_header_t *header)
{
	// cursed $!# pointer arithmetic and casting so bad I prolly broke the ABI
	return *(memory_pool_t **)(((char *)header + header->chunk_size) - ((DEADZONE_PADDING / 2) * 3));
	// 	pool_header_t *current_head = header;
	// reloop:
	// 	if (current_head->bitflags & F_FIRST_HEAD) {
	// 		return ((memory_pool_t *)((char *)current_head - PD_RESERVED_F_SIZE));
	// 	}
	//
	// 	// If this goes past the boundary and out of bounds, it should terminate instead of infinitely looping.
	// 	const u32 deadzone_prev_size = *((u32 *)(((char *)current_head + current_head->chunk_size) - (DEADZONE_PADDING / 2)));
	// 	if (deadzone_prev_size == 0 || deadzone_prev_size > MAX_FIRST_POOL_SIZE) {
	// 		return nullptr;
	// 	}
	//
	// 	current_head = (pool_header_t *)((char *)current_head - deadzone_prev_size);
	// 	goto reloop;
}


int free_node_add(memory_pool_t *pool, pool_free_node_t *free_node)
{
	if (pool->first_free == nullptr) {
		pool->first_free = free_node;
		goto done;
	}
	// for now we just add new frees to the end. Might not be temporary.

	pool_free_node_t *current_node = pool->first_free;
	while (current_node->next_free != nullptr) {
		current_node = current_node->next_free;
	}

	current_node->next_free = free_node;
done:
	pool->free_count++;
	return 0;
}


// TODO implement free_node_remove(),
//int free_node_remove(memory_pool_t *pool, pool_free_node_t *free_node)
//{
//	return 0;
//}


inline bool handle_generation_checksum(const syn_handle_t *restrict hdl)
{
	return ((return_handle(hdl->header->handle_matrix_index))->generation == hdl->generation);
}


inline void update_table_generation(const u32 encoded_matrix_index)
{
	(return_handle(encoded_matrix_index))->generation++;
}


void table_destructor()
{
	handle_table_t *table_arr[arena_thread->table_count];
	const int table_arr_len = return_table_array(table_arr);

	if (table_arr_len == -1) {
		return;
	}

	for (int i = 0; i < table_arr_len; i++) {
		#ifdef ALLOC_DEBUG
		sync_alloc_log.to_console(log_stdout,
		                          "destroying table at: %p\n",
		                          table_arr[i]);
		#endif
		munmap(table_arr[i], PD_HDL_MATRIX_SIZE);
	}
	arena_thread->table_count = 0;
	arena_thread->first_hdl_tbl = nullptr;
}


inline int return_table_array(handle_table_t **arr)
{
	if (arena_thread->table_count == 0 || arena_thread->first_hdl_tbl == nullptr) {
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
#endif


pool_header_t *return_header(void *block_ptr)
{
	int search_attempts = 0;
	int offset = PD_HEAD_SIZE;
recheck:

	pool_header_t *header_candidate = (pool_header_t *)((u8 *)block_ptr - offset);

	// This should search 3 times for the max header offset value of ALIGNMENT.
	if (search_attempts > 2) {
		return nullptr;
	}

	const bool is_invalid_header = (header_candidate->chunk_size > 0 &&
	                                header_candidate->chunk_size < MAX_ALLOC_POOL_SIZE) == 0;

	if (is_invalid_header) {
		offset *= 2;
		search_attempts++;
		goto recheck;
	}
	return header_candidate;
}


int syn_unmap_page(void *restrict mem, const usize bytes)
{
	return munmap(mem, bytes);
}


void *syn_map_page(const usize bytes)
{
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}


inline int return_pool_array(memory_pool_t **arr)
{
	memory_pool_t *pool = arena_thread->first_mempool;

	int idx = 0;
	for (; idx < arena_thread->pool_count; idx++) {
		arr[idx] = pool;
		pool = pool->next_pool;
	}
	return idx;
}


inline int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool)
{
	pool_free_node_t *head = pool->first_free;

	int idx = 0;
	while (idx < pool->free_count && head != nullptr) {
		arr[idx++] = head;
		head = head->next_free;
	}

	return idx;
}


void update_sentinel_and_free_flags(pool_header_t *head)
{
	const u32 prev_block_size = (head->bitflags & F_FIRST_HEAD)
	                            ? 0
	                            : (u32)(*((u8 *)head + head->chunk_size)) - (DEADZONE_PADDING / 2);

	constexpr u32 chunk_overflow = (KIBIBYTE * 128) + 1;

	// Branch inversion doesnt work for bitmaps (or maybe I just dont know how),
	// so it gets messy here. At least bitflags line up between pool_header_t and
	// pool_free_node_t, so no casting is needed for setting their bitflags.

	if (prev_block_size > 0 && prev_block_size < chunk_overflow) {
		pool_header_t *prev_head = (pool_header_t *)((u8 *)head - prev_block_size);
		if (prev_head->bitflags & F_SENTINEL) {
			prev_head->bitflags &= ~F_SENTINEL;
			head->bitflags |= F_SENTINEL;
		}
		if (prev_head->bitflags & F_FREE) {
			head->bitflags |= F_PREV_FREE;
		}
		if (head->bitflags & F_FREE) {
			prev_head->bitflags |= F_NEXT_FREE;
		}
		// TODO extract a lot of these branches into sub functions
		// TODO implement free list updating
	}
	if (head->bitflags & F_SENTINEL) {
		return;
	}

	pool_header_t *next_head = (pool_header_t *)((u8 *)head + head->chunk_size);
	if (next_head->bitflags == 0 || next_head->bitflags & F_SENTINEL) {
		return;
	}

	if (next_head->bitflags & F_SENTINEL && head->bitflags & F_SENTINEL) {
		head->bitflags &= ~F_SENTINEL;
	}
	if (next_head->bitflags & F_FREE) {
		head->bitflags |= F_NEXT_FREE;
	}
	if (head->bitflags & F_FREE) {
		next_head->bitflags |= F_PREV_FREE;
	}
}


void pool_destructor()
{
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);
	if (!pool_arr_len) {
		return;
	}
	for (int i = pool_arr_len - 1; i >= 0; i--) {
		#ifdef ALLOC_DEBUG
		if (i != 0) {
			sync_alloc_log.to_console(log_stdout,
			                          "destroying pool of size: %lu at: %p\n",
			                          pool_arr[i]->size,
			                          pool_arr[i]->heap_base);
		} else {
			sync_alloc_log.to_console(log_stdout, "destroying core: %p\n", arena_thread);
		}
		#endif
		munmap(pool_arr[i]->heap_base, pool_arr[i]->size);
	}
}
