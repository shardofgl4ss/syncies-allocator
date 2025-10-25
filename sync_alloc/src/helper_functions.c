//
// Created by SyncShard on 10/15/25.
//

#include "helper_functions.h"

extern _Thread_local Arena *arena_thread;

bool
mp_helper_handle_generation_checksum(const struct Arena_Handle *restrict hdl)
{
	const usize row = mp_helper_return_matrix_row(hdl);
	const usize col = mp_helper_return_matrix_col(hdl);
	const Handle_Table *table = arena_thread->first_hdl_tbl;

	if (row == 1)
		goto checksum;
	for (usize i = 1; i < row; i++)
		table = table->next_table;

checksum:
	if (table->handle_entries[col].generation != hdl->generation)
		return false;
	return true;
}

void
mp_helper_update_table_generation(const struct Arena_Handle *restrict hdl)
{
	if (hdl == nullptr)
		return;
	const usize row = mp_helper_return_matrix_row(hdl);
	const usize col = mp_helper_return_matrix_col(hdl);

	Handle_Table *table = arena_thread->first_hdl_tbl;

	if (row == 1)
		goto update_generation;
	for (usize i = 1; i < row; i++)
		table = table->next_table;

update_generation:
	table->handle_entries[col].generation++;
}
