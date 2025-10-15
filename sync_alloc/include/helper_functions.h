//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include "defs.h"
#include "structs.h"

inline static void *
mp_helper_map_mem(const usize bytes)
{
	return mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

inline static usize
mp_helper_add_padding(const usize input) { return (input + (ALIGNMENT - 1)) & (usize)~(ALIGNMENT - 1); }

inline static void
mp_helper_destroy(void *restrict mem, const size_t bytes)
{
	const int x = munmap(mem, bytes);
	if (x == -1)
		perror("error: arena destruction failure! this should never happen, there is probably lost memory!\n");
}


/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
inline static void *
mp_helper_return_block_addr(Pool_Header *head) { return (char *)head + PD_HEAD_SIZE; }

inline static Arena *
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

inline static void
mp_helper_calculate_matrix(const Arena_Handle *restrict hdl, usize *restrict row, usize *restrict col)
{
	if (row == nullptr || col == nullptr || hdl == nullptr)
		return;
	*row = hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS;
	*col = hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS;
}

inline static bool
mp_helper_handle_generation_checksum(const Arena *restrict arena, const Arena_Handle *restrict hdl)
{
	usize row, col;
	mp_helper_calculate_matrix(hdl, &row, &col);

	const Handle_Table *table = arena->first_hdl_tbl;

	if (row <= 1)
		goto checksum;
	for (usize i = 0; i < row; i++)
		table = table->next_table;

checksum:
	if (table->handle_entries[col].generation != hdl->generation)
		return false;
	return true;
}

inline static void
mp_helper_update_table_generation(const Arena_Handle *restrict hdl)
{
	if (hdl == nullptr)
		return;
	const Arena *restrict arena = mp_helper_return_base_arena(hdl);

	usize row, col;
	mp_helper_calculate_matrix(hdl, &row, &col);

	Handle_Table *table = arena->first_hdl_tbl;

	if (row == 0)
		goto update_generation;
	for (usize i = 0; i < row; i++)
		table = table->next_table;

update_generation:
	table->handle_entries[col].generation++;
}

#endif //ARENA_ALLOCATOR_HELPER_FUNCTIONS_H