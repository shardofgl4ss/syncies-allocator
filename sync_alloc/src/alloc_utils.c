//
// Created by SyncShard on 10/26/25.
//

#include "alloc_utils.h"
#include "alloc_init.h"
#include "include/debug.h"
#include "defs.h"
#include "handle.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"
#include <stdint.h>
#include <sys/mman.h>

// Not implemented
// void
// defragment_pool(Memory_Pool *pool);

#ifdef SYN_USE_RAW

inline int bad_alloc_check(const void *, const int do_checksum)
{
}

#else


static syn_handle_t *index_table_from_header(pool_header_t *header, const handle_table_t **table)
{
}

static syn_handle_t *index_table_from_handle(syn_handle_t *handle, const handle_table_t **table)
{
}


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


bool handle_generation_checksum(const syn_handle_t *restrict hdl)
{
	const usize row = matrix_row_from_handle(hdl);
	const usize col = matrix_col_from_handle(hdl);
	const handle_table_t *table = arena_thread->first_hdl_tbl;

	if (row == 1) {
		goto checksum;
	}
	for (usize i = 1; i < row; i++) {
		table = table->next_table;
	}

checksum:
	return (table->handle_entries[col].generation == hdl->generation);
}


void update_table_generation(const syn_handle_t *restrict hdl)
{
	if (hdl == nullptr) {
		return;
	}
	const usize row = matrix_row_from_handle(hdl);
	const usize col = matrix_col_from_handle(hdl);

	handle_table_t *table = arena_thread->first_hdl_tbl;

	if (row == 1) {
		goto update_generation;
	}
	for (usize i = 1; i < row; i++) {
		table = table->next_table;
	}

update_generation:
	table->handle_entries[col].generation++;
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
}


inline int return_table_array(handle_table_t **arr)
{
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
#endif


pool_header_t *return_header(void *block_ptr)
{
	if (!block_ptr) {
		return nullptr;
	}
	const pool_header_t *header_candidate = nullptr;
	int search_attempts = 0;
	int search_offset = PD_HEAD_SIZE;
recheck:

	header_candidate = (pool_header_t *)((u8 *)block_ptr - search_offset);

	bool is_invalid_header = header_candidate->chunk_size
	if () {

	}
}

syn_handle_t return_handle(pool_header_t *header)
{
	const usize row = header->handle_idx / MAX_TABLE_HNDL_COLS;
	const usize col = header->handle_idx % MAX_TABLE_HNDL_COLS;

	handle_table_t *table = arena_thread->first_hdl_tbl;

	if (row == 1) {
		goto done;
	}
	for (usize i = 1; i < row; i++) {
		table = table->next_table;
	}
done:

}


int syn_unmap_page(void *restrict mem, const usize bytes)
{
	return munmap(mem, bytes);
}


void *syn_map_page(const usize bytes)
{
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}


inline int return_pool_array(memory_pool_t **arr)
{
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


inline int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool)
{
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
