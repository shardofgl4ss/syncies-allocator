//
// Created by SyncShard on 10/15/25.
//

#include "helper_functions.h"

static Arena *
mp_helper_return_base_arena(const Arena_Handle *restrict user_handle)
{
	if (user_handle == nullptr)
		return nullptr;

	Pool_Header *head = user_handle->header;

	const Pool_Header *prev_head = (head->prev_block_size != 0) ? head - head->prev_block_size : nullptr;
	if (prev_head == nullptr)
		goto found_last_head;
	while (prev_head->prev_block_size != 0)
		prev_head = head - head->prev_block_size;

	found_last_head:
		return (Arena *)((char *)head - (PD_POOL_SIZE + PD_ARENA_SIZE));
}

static bool
mp_helper_handle_generation_checksum(const Arena *restrict arena, const Arena_Handle *restrict hdl)
{
	const usize row = mp_helper_return_matrix_row(hdl);
	const usize col = mp_helper_return_matrix_col(hdl);
	const Handle_Table *table = arena->first_hdl_tbl;

	if (row == 0)
		goto checksum;
	for (usize i = 0; i < row; i++)
		table = table->next_table;

	checksum:
		if (table->handle_entries[col].generation != hdl->generation)
			return false;
	return true;
}

static void
mp_helper_update_table_generation(const Arena_Handle *restrict hdl)
{
	if (hdl == nullptr)
		return;
	const Arena *restrict arena = mp_helper_return_base_arena(hdl);

	const usize row = mp_helper_return_matrix_row(hdl);
	const usize col = mp_helper_return_matrix_col(hdl);

	Handle_Table *table = arena->first_hdl_tbl;

	if (row == 0)
		goto update_generation;
	for (usize i = 0; i < row; i++)
		table = table->next_table;

	update_generation:
		table->handle_entries[col].generation++;
}