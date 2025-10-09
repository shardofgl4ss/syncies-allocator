//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

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
mp_destroy(void *restrict mem, const size_t bytes)
{
	static int index = 0;
	const int x = munmap(mem, bytes);

	if (x == -1) { goto destroy_fail; }

	printf("pool %d: destroyed\n", ++index);
	return;

destroy_fail:
	perror("error: arena destruction failure! this should never happen, there is probably lost memory!\n");
	fflush(stdout);
}

inline static void *
return_vptr(Mempool_Header *head) { return (char *)head + PD_HEAD_SIZE; }

inline static Arena *
return_base_arena(const Arena_Handle *user_handle)
{
	Mempool_Header *head = user_handle->header;

	if (head->previous_header) { while (head->previous_header) { head = head->previous_header; } }
	return (Arena *)((char *)head - (PD_POOL_SIZE + PD_ARENA_SIZE));
}

inline static bool
mempool_handle_generation_lookup(const Arena *arena, const Arena_Handle *user_handle)
{
	const size_t row_index = user_handle->handle_matrix_index / TABLE_MAX_COL;
	const size_t col_index = user_handle->handle_matrix_index % TABLE_MAX_COL;

	if (arena->handle_table[row_index]->handle_entries[col_index].generation != user_handle->generation)
	{
		return false;
	}
	return true;
}

#endif //ARENA_ALLOCATOR_HELPER_FUNCTIONS_H