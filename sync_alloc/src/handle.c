//
// Created by SyncShard on 10/15/25.
//

#include "handle.h"
#include "alloc_utils.h"
#include "defs.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"
#include <stdbit.h>

// TODO refactor handle functions to use new internal struct VLA apis,
// for more contained tracking of each object

typedef struct Handle_Context {
	handle_table_t **table_arr;
	i32 max_table_array_index;
	i32 current_index;
} __attribute__((aligned(16))) handle_context_t;


handle_table_t *new_handle_table()
{
	handle_table_t *new_tbl = helper_map_mem(PD_HDL_MATRIX_SIZE);
	if (!new_tbl) {
		return nullptr;
	}

	const bool no_first_table = (!arena_thread->first_hdl_tbl || !arena_thread->table_count) != 0;
	if (no_first_table) {
		arena_thread->first_hdl_tbl = new_tbl;
		new_tbl->next_table = nullptr;
	}
	else {
		handle_table_t *table_array[arena_thread->table_count];
		const i32 max_array_length = return_table_array(table_array);
		table_array[max_array_length - 1]->next_table = new_tbl;
	}

	new_tbl->entries_bitmap = 0;
	new_tbl->table_id = ++arena_thread->table_count;

	return new_tbl;
}


static i32 find_non_empty_table(handle_context_t *ctx)
{
	bool retried = false;
reloop:
	for (; ctx->current_index < ctx->max_table_array_index; ctx->current_index++) {
		const handle_table_t *current_table = ctx->table_arr[ctx->current_index];
		if (stdc_count_ones_ull(current_table->entries_bitmap) != MAX_TABLE_HNDL_COLS) {
			return ctx->current_index;
		}
	}
	if (retried) {
		return -1;
	}
	handle_table_t *new_table = new_handle_table();
	if (new_table == nullptr) {
		return -1;
	}

	ctx->table_arr[ctx->max_table_array_index] = new_table;
	ctx->max_table_array_index++;
	ctx->current_index = ctx->max_table_array_index;
	retried = true;
	goto reloop;
}


syn_handle_t create_handle_and_entry(pool_header_t *head)
{
	handle_context_t ctx = {};

	if (!arena_thread->table_count && !new_handle_table()) {
		return invalid_hdl();
	}

	handle_table_t *table_arr[arena_thread->table_count + 1];
	ctx.max_table_array_index = return_table_array(table_arr);
	if (!ctx.max_table_array_index) {
		return invalid_hdl();
	}
	ctx.table_arr = table_arr;

	const i32 new_index = find_non_empty_table(&ctx);
	if (new_index == -1) {
		return invalid_hdl();
	}
	handle_table_t *non_empty_table = table_arr[new_index];
	i32 free_handle_column = stdc_first_trailing_zero_ull(non_empty_table->entries_bitmap);
	if (free_handle_column == 0) {
		return invalid_hdl();
	}
	free_handle_column--;
	//syn_handle_t new_hdl = non_empty_table->handle_entries[free_handle_column];
	non_empty_table->entries_bitmap |= (1ULL << free_handle_column);

	syn_handle_t new_hdl = {
		new_hdl.addr = (void *)BLOCK_ALIGN_PTR(head, ALIGNMENT),
		new_hdl.header = head,
		new_hdl.generation = 1,
		new_hdl.handle_matrix_index = (arena_thread->table_count * MAX_TABLE_HNDL_COLS) + free_handle_column,
	};

	table_arr[new_index]->handle_entries[free_handle_column] = new_hdl;

	return new_hdl;
}
