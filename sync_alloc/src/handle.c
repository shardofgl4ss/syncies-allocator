//
// Created by SyncShard on 10/15/25.
//

#include "handle.h"

static Arena_Handle
mempool_create_handle_and_entry(Arena *restrict arena, Pool_Header *restrict head)
{
	Arena_Handle hdl = {};
	Handle_Table *table = arena->first_hdl_tbl;
	if (head == nullptr || arena == nullptr || arena->first_mempool == nullptr) { return hdl; }

reloop_hdl_tbl:
	while (MAX_TABLE_HNDL_COLS == __builtin_popcountll(table->entries_bitmap))
	{
		if (table->next_table != nullptr)
			table = table->next_table;
		else
		{
			table->next_table = mempool_new_handle_table(arena, table);
			if (table->next_table != nullptr)
				return hdl;
			table = table->next_table;
		}
	}

	const i32 handle_idx = __builtin_ctzll(~table->entries_bitmap);
	if (handle_idx == -1 || handle_idx < 0)
		goto reloop_hdl_tbl;
	table->entries_bitmap |= (1ull << handle_idx);

	hdl.header = head;
	hdl.addr = (void *)((char *)head + PD_HEAD_SIZE);
	hdl.handle_matrix_index = arena->table_count * MAX_TABLE_HNDL_COLS + (u32)handle_idx;
	hdl.generation = 1;

	table->handle_entries[handle_idx] = hdl;
	head->handle_idx = hdl.handle_matrix_index;
	return hdl;
}


static Handle_Table *
mempool_new_handle_table(Arena *restrict arena, Handle_Table *restrict table)
{
	if (table == nullptr)
		goto fail_null;

	Handle_Table *new_tbl = mp_helper_map_mem(PD_HDL_MATRIX_SIZE);

	if (new_tbl == nullptr)
		goto fail_alloc;

	const u32 new_id = ++arena->table_count;
	table->next_table = new_tbl;

	new_tbl->entries_bitmap = 0;
	new_tbl->table_id = new_id;

	return new_tbl;
fail_alloc:
	fprintf(stderr, "ERR_NO_MEMORY: failed to allocate table: %hu\n", new_id);
fail_null:
	return nullptr;
}