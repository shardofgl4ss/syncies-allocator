//
// Created by SyncShard on 10/26/25.
//

#include "alloc_utils.h"
#include "alloc_init.h"
#include "debug.h"
#include "defs.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"
#include <sys/mman.h>

// Not implemented
// void
// defragment_pool(Memory_Pool *pool);

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
		auto *prev_head = (pool_header_t *)((u8 *)head - prev_block_size);
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

	auto *next_head = (pool_header_t *)((u8 *)head + head->chunk_size);
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


bool handle_generation_checksum(const syn_handle_t *restrict hdl)
{
	const usize row = helper_return_matrix_row(hdl);
	const usize col = helper_return_matrix_col(hdl);
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
	const usize row = helper_return_matrix_row(hdl);
	const usize col = helper_return_matrix_col(hdl);

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
