//
// Created by SyncShard on 10/15/25.
//

#include "handle.h"
#include "alloc_init.h"
#include "debug.h"
#include "helper_functions.h"
#include <stdbit.h>
#include <stdint.h>

extern _Thread_local Arena *arena_thread;

struct Arena_Handle
mempool_create_handle_and_entry(Pool_Header *restrict head)
{
	if (arena_thread == nullptr) {
		#if defined(ALLOC_DEBUG)
		sync_alloc_log.to_console(
			log_stderr,
			"PANICKING: arena_thread TLS is nullptr at: mempool_create_handle_and_entry()!\n"
		);
		#endif
		arena_panic();
	}
	struct Arena_Handle hdl = {};
	if (arena_thread->first_hdl_tbl == nullptr)
		mempool_new_handle_table(nullptr);

	Handle_Table *table = arena_thread->first_hdl_tbl;

	if (head == nullptr || arena_thread->first_mempool == nullptr) {
		hdl.generation = UINT32_MAX;
		#if defined(ALLOC_DEBUG)
		sync_alloc_log.to_console(
			log_stderr,
			"invalid parameters received for function: mempool_create_handle_and_entry()\n"
		);
		#endif
		return hdl;
	}

	bit64 entries_bitcount = stdc_count_ones(table->entries_bitmap);
reloop_hdl_tbl:
	while (entries_bitcount == MAX_TABLE_HNDL_COLS) {
		if (table->next_table != nullptr) {
			table = table->next_table;
			entries_bitcount = stdc_count_ones_ull(table->entries_bitmap);
		}
		else {
			table->next_table = mempool_new_handle_table(table);
			if (table->next_table != nullptr)
				return hdl;
			table = table->next_table;
			entries_bitcount = stdc_count_ones_ull(table->entries_bitmap);
		}
	}

	u32 handle_idx = stdc_first_trailing_one(~table->entries_bitmap);
	if (handle_idx == 0)
		goto reloop_hdl_tbl;
	--handle_idx;
	table->entries_bitmap |= (1ull << handle_idx);

	hdl.header = head;
	hdl.addr = (void *)((char *)head + PD_HEAD_SIZE);
	hdl.handle_matrix_index = arena_thread->table_count * MAX_TABLE_HNDL_COLS + handle_idx;
	hdl.generation = 1;

	table->handle_entries[handle_idx] = hdl;
	head->handle_idx = hdl.handle_matrix_index;
	return hdl;
}


Handle_Table *
mempool_new_handle_table(Handle_Table *restrict table)
{
	if (arena_thread == nullptr) {
		#if defined(ALLOC_DEBUG)
		sync_alloc_log.to_console(
			log_stderr,
			"PANICKING: arena_thread TLS is nullptr at function: mempool_new_handle_table()!\n"
		);
		#endif
		arena_panic();
	}

	Handle_Table *new_tbl = helper_map_mem(PD_HDL_MATRIX_SIZE);

	const u32 new_id = ++arena_thread->table_count;

	if (new_tbl == nullptr)
		goto fail_alloc;

	if (table == nullptr)
		arena_thread->first_hdl_tbl = new_tbl;
	else
		table->next_table = new_tbl;

	new_tbl->entries_bitmap = 0;
	new_tbl->table_id = new_id;

	return new_tbl;

fail_alloc:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "failed to allocate table: %hu\n", new_id);
	#endif
	return nullptr;
}
