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

inline int bad_alloc_check(const struct Arena_Handle *restrict hdl, const int do_checksum) {
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
	if (hdl->header->bitflags & PH_FROZEN) {
		return 2;
	}
	#endif
	return 0;
}


void update_sentinel_flags(pool_header_t *head) {
	const u32 prev_block_size = (u32)(*((char *)head - (DEADZONE_PADDING / 2)));
	if (!(head->bitflags & PH_SENTINEL_F)) {
		pool_header_t *prev_head = head - prev_block_size;
		if (prev_head != nullptr && prev_head->bitflags & PH_FREE) {
			head->bitflags |= PH_PREV_FREE;
			prev_head->bitflags |= PH_NEXT_FREE;
		}
	}
	if (!(head->bitflags & PH_SENTINEL_L)) {
		pool_header_t *next_head = head + head->size;
		if (next_head != nullptr && next_head->bitflags & PH_FREE) {
			head->bitflags |= PH_NEXT_FREE;
			next_head->bitflags |= PH_PREV_FREE;
		}
	}
}


bool handle_generation_checksum(const struct Arena_Handle *restrict hdl) {
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


void update_table_generation(const struct Arena_Handle *restrict hdl) {
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


void table_destructor() {
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


void pool_destructor() {
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);
	for (int i = 0; i < pool_arr_len; i++) {
		#ifdef ALLOC_DEBUG
		sync_alloc_log.to_console(log_stdout,
		                          "destroying pool of size: %lu at: %p\n",
		                          pool_arr[i]->size,
		                          pool_arr[i]->mem);
		#endif
		munmap(pool_arr[i]->mem, pool_arr[i]->size);
	}
}
