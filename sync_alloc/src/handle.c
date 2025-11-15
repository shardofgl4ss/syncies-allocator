//
// Created by SyncShard on 10/15/25.
//

#include "handle.h"
#include "alloc_utils.h"
#include "defs.h"
#include "structs.h"
#include "types.h"
#include <stdbit.h>

// clang-format off
//typedef struct Handle_Context {
//	handle_table_t **table_arr;
//	i32 max_table_array_index;
//	i32 current_index;
//} __attribute__((aligned (16))) handle_context_t;
// clang-format on

handle_table_t *new_handle_table()
{
	handle_table_t *new_tbl = syn_map_page(PD_HDL_MATRIX_SIZE);
	if (!new_tbl) {
		return nullptr;
	}

	const bool no_first_table = (arena_thread->first_hdl_tbl == nullptr ||
	                             !arena_thread->table_count) != 0;
	if (no_first_table) {
		arena_thread->first_hdl_tbl = new_tbl;
	} else {
		handle_table_t *temp = arena_thread->first_hdl_tbl;
		for (int i = 0; i < arena_thread->table_count - 1; i++) {
			temp = temp->next_table;
		}
		temp->next_table = new_tbl;
	}

	new_tbl->entries_bitmap = 0;
	new_tbl->table_id = ++arena_thread->table_count;
	new_tbl->next_table = nullptr;

	return new_tbl;
}


[[maybe_unused]]
static handle_table_t *index_table(const i32 row)
{
	handle_table_t *tbl = arena_thread->first_hdl_tbl;

	if (row == -1) {
		while (tbl->next_table) {
			tbl = tbl->next_table;
		}
		return tbl;
	}
	for (int i = 1; i < row; i++) {
		tbl = tbl->next_table;
	}
	return tbl;
}


static handle_table_t *find_non_empty_table()
{
	bool retried = false;
	handle_table_t *current_table = arena_thread->first_hdl_tbl;
reloop:
	for (int i = 0; i < arena_thread->table_count; i++) {
		if (current_table == nullptr) {
			break;
		}
		if (stdc_count_ones_ull(current_table->entries_bitmap) != MAX_TABLE_HNDL_COLS) {
			return current_table;
		}
		current_table = current_table->next_table;
	}

	if (retried) {
		return nullptr;
	}

	handle_table_t *new_table = new_handle_table();
	if (new_table == nullptr) {
		return nullptr;
	}

	retried = true;
	current_table = new_table;
	goto reloop;
}


syn_handle_t create_handle_and_entry(pool_header_t *head)
{
	//handle_context_t ctx = {};

	if (!arena_thread->table_count && !new_handle_table()) {
		return invalid_block();
	}

	//handle_table_t *table_arr[arena_thread->table_count + 1];
	//ctx.max_table_array_index = return_table_array(table_arr);

	//if (!ctx.max_table_array_index) {
	//	return invalid_block();
	//}

	//ctx.table_arr = table_arr;
	handle_table_t *table = find_non_empty_table();

	if (table == nullptr) {
		return invalid_block();
	}

	i32 free_handle_column = stdc_first_trailing_zero_ull(table->entries_bitmap);

	if (free_handle_column == 0) {
		return invalid_block();
	}

	free_handle_column--;
	table->entries_bitmap |= (1ULL << free_handle_column);

	syn_handle_t new_hdl = {
		new_hdl.addr = (void *)BLOCK_ALIGN_PTR(head, ALIGNMENT),
		new_hdl.header = head,
		new_hdl.generation = 1,
		//new_hdl.handle_matrix_index = (arena_thread->table_count * MAX_TABLE_HNDL_COLS) + free_handle_column,
	};

	#ifndef SYN_USE_RAW
	head->handle_matrix_index = ((table->table_id - 1) * MAX_TABLE_HNDL_COLS) + free_handle_column;
	#endif


	table->handle_entries[free_handle_column] = new_hdl;

	return new_hdl;
}