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
mempool_map_mem(const size_t bytes)
{
	return mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

inline static size_t
mempool_add_padding(const size_t input) { return (input + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1); }

inline static void
mempool_destroy(void *restrict mem, const size_t bytes)
{
	const int x = munmap(mem, bytes);
	if (x == -1)
		perror("error: arena destruction failure! this should never happen, there is probably lost memory!\n");
}


/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
inline static void *
return_vptr(Pool_Header *head) { return (char *)head + PD_HEAD_SIZE; }

inline static Arena *
return_base_arena(const Arena_Handle *restrict user_handle)
{
	Pool_Header *head = user_handle->header;
	ptrdiff_t offset =

	if (head->prev_header) { while (head->prev_header) { head = head->prev_header; } }
	return (Arena *)((char *)head - (PD_POOL_SIZE + PD_ARENA_SIZE));
}

inline static bool
mempool_handle_generation_checksum(const Arena *arena, const Arena_Handle *user_handle)
{
	const size_t row_index = user_handle->handle_matrix_index / TABLE_MAX_COL;
	const size_t col_index = user_handle->handle_matrix_index % TABLE_MAX_COL;


	if (arena->first_hdl_tbl[row_index]->handle_entries[col_index].generation != user_handle->generation)
	{
		return false;
	}
	return true;
}

inline static void
mempool_update_table_generation(const Arena_Handle *restrict hdl)
{
	const Arena *restrict arena = return_base_arena(hdl);
	const size_t row = hdl->handle_matrix_index / TABLE_MAX_COL;
	const size_t col = hdl->handle_matrix_index % TABLE_MAX_COL;

	arena->first_hdl_tbl[row]->handle_entries[col].generation++;
}

#endif //ARENA_ALLOCATOR_HELPER_FUNCTIONS_H