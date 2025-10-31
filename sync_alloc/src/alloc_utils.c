//
// Created by SyncShard on 10/26/25.
//

#include "alloc_init.h"
#include "alloc_utils.h"
#include "debug.h"
#include "defs.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include "structs.h"

// Not implemented
// void
// defragment_pool(Memory_Pool *pool);

inline int bad_alloc_check(struct Arena_Handle *restrict hdl) {
	if (arena_thread == nullptr) {
		syn_panic("core arena context was lost!\n");
	}
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (corrupt_header_check(hdl->header)) {
		syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	}
	if (!handle_generation_checksum(hdl)) {
		sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
		return 1;
	}
	if (hdl->header->block_flags & PH_FROZEN) {
		return 2;
	}
	#endif
	return 0;
}

void update_sentinel_flags(Pool_Header *head) {
	u32 prev_block_size = (u32)(*((char *)head - (DEADZONE_PADDING / 2)));
	if (!(head->block_flags & PH_SENTINEL_F)) {
		Pool_Header *prev_head = head - prev_block_size;
		if (prev_head != nullptr && prev_head->block_flags & PH_FREE) {
			head->block_flags |= PH_PREV_FREE;
			prev_head->block_flags |= PH_NEXT_FREE;
		}
	}
	if (!(head->block_flags & PH_SENTINEL_L)) {
		Pool_Header *next_head = head + head->size;
		if (next_head != nullptr && next_head->block_flags & PH_FREE) {
			head->block_flags |= PH_NEXT_FREE;
			next_head->block_flags |= PH_PREV_FREE;
		}
	}
}

bool handle_generation_checksum(const struct Arena_Handle *restrict hdl) {
	const usize row = helper_return_matrix_row(hdl);
	const usize col = helper_return_matrix_col(hdl);
	const Handle_Table *table = arena_thread->first_hdl_tbl;

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

	Handle_Table *table = arena_thread->first_hdl_tbl;

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
	Handle_Table *table = arena_thread->first_hdl_tbl;
	for (u32 i = 1; i < arena_thread->table_count; i++) {
		if (table == nullptr) {
			return;
		}
		Handle_Table *table_tmp = table;
		table = table->next_table;

		#ifdef ALLOC_DEBUG
		sync_alloc_log.to_console(log_stdout, "destroying table: %u at addr: %p\n", i, table_tmp);
		#endif

		helper_destroy(table_tmp, PD_HDL_MATRIX_SIZE);
	}
}

void pool_destructor() {
	const Memory_Pool *pool = arena_thread->first_mempool;

	if (arena_thread->pool_count == 1) {
		#ifdef ALLOC_DEBUG
		sync_alloc_log.to_console(log_stdout, "destroying pool: 1 at addr: %p\n", pool->mem);
		#endif

		helper_destroy(pool->mem, PD_POOL_SIZE);
		return;
	}

	pool = pool->next_pool;

	#ifdef ALLOC_DEBUG
	sync_alloc_log.to_console(log_stdout, "destroying pool: 1 at addr: %p\n", pool->mem);
	#endif

	helper_destroy(pool->prev_pool->mem, PD_POOL_SIZE);

	for (u32 i = 2; i < arena_thread->pool_count; i++) {
		if (pool->next_pool == nullptr) {
			#ifdef ALLOC_DEBUG
			sync_alloc_log.to_console(
				log_stdout, "destroying pool: %u at addr: %p\n", i, pool->prev_pool->mem);
			#endif

			helper_destroy(pool->mem, PD_POOL_SIZE);
		};
		pool = pool->next_pool;

		#ifdef ALLOC_DEBUG
		sync_alloc_log.to_console(log_stdout, "destroying pool: %u at addr: %p\n", i, pool->prev_pool->mem);
		#endif

		helper_destroy(pool->mem, PD_POOL_SIZE);
	}
}
