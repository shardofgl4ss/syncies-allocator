//
// Created by SyncShard on 10/15/25.
//

#include "handle.h"
#include "alloc_utils.h"
#include "debug.h"
#include "defs.h"
#include "structs.h"
#include "types.h"
#include <stdbit.h>


inline bool handle_generation_checksum(const syn_handle_t *restrict hdl)
{
	return ((return_handle(hdl->header->handle_matrix_index))->generation == hdl->generation);
}



inline void update_table_generation(const u32 encoded_matrix_index)
{
	(return_handle(encoded_matrix_index))->generation++;
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
		syn_unmap_page(table_arr[i], PD_HDL_MATRIX_SIZE);
	}
	arena_thread->table_count = 0;
	arena_thread->first_hdl_tbl = nullptr;
}


inline int return_table_array(handle_table_t **arr)
{
	if (arena_thread->table_count == 0 || arena_thread->first_hdl_tbl == nullptr) {
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


inline syn_handle_t *return_handle(const u32 encoded_matrix_index)
{
	const u32 row = encoded_matrix_index / MAX_TABLE_HNDL_COLS;
	const u32 col = encoded_matrix_index % MAX_TABLE_HNDL_COLS;

	handle_table_t *table = arena_thread->first_hdl_tbl;

	for (u32 i = 0; i < row; i++) {
		table = table->next_table;
	}

	return &table->handle_entries[col];
}


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
	if (!arena_thread->table_count && !new_handle_table()) {
		return invalid_block();
	}

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
	};

	#ifndef SYN_USE_RAW
	head->handle_matrix_index = ((table->table_id - 1) * MAX_TABLE_HNDL_COLS) + free_handle_column;
	#endif


	table->handle_entries[free_handle_column] = new_hdl;

	return new_hdl;
}