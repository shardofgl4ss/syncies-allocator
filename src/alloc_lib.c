//
// Created by SyncShard on 10/9/25.
//

#include "../include/alloc_lib.h"

#include <string.h>


Arena *
arena_create()
{
	void *raw_pool = mempool_map_mem(FIRST_POOL_ALLOC + PD_RESERVED_F_SIZE);

	if (raw_pool == MAP_FAILED) { goto mp_create_error; }

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set mem_size to the FIRST_ARENA_POOL_SIZE.				*
	 *	This spoofs a max memory of 4096 when its actually 4096 + reserved.							*
	 *  Alignment is added to Mempool in case the user changes it to 16 or 32, it will be aligned.	*
	 *  The alignment however adds some complexity later on.										*/

	Arena *arena = raw_pool;
	Mempool *first_pool = (Mempool *)((char *)raw_pool + sizeof(Arena));

	first_pool->mem = (char *)raw_pool + PD_RESERVED_F_SIZE;
	first_pool->mem_offset = 0;
	first_pool->mem_size = FIRST_POOL_ALLOC;
	first_pool->next_pool = NULL;
	first_pool->prev_pool = NULL;

	arena->total_mem_size = FIRST_POOL_ALLOC;
	arena->table_count = 0;
	arena->handle_table[arena->table_count++] = mempool_new_handle_table(arena);

	return arena;

mp_create_error:
	perror("ERR_NO_MEMORY: arena creation failed!\n");
	fflush(stdout);
	return NULL;
}


void
arena_destroy(Arena *restrict arena)
{
	if (!arena) { return; }
	const Mempool *mp = arena->first_mempool;

	if (!mp->next_pool)
	{
		mp_destroy(mp->mem, mp->mem_size + PD_RESERVED_F_SIZE);
		printf("arena destroyed\n");
		fflush(stdout);
	}

	mp = mp->next_pool;
	mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + PD_RESERVED_F_SIZE);

	while (mp->next_pool)
	{
		mp = mp->next_pool;
		mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + PD_POOL_SIZE);
	}

	mp_destroy(mp->mem, mp->mem_size + PD_POOL_SIZE);

	printf("arena destroyed\n");
	fflush(stdout);
}


static void
arena_defragment(Arena *arena, Defrag_Type defrag)
{
	// wip
}


Arena_Handle
arena_alloc(Arena *arena, const size_t size)
{
	Arena_Handle user_handle = {
		.addr = NULL,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = NULL,
	};

	if (arena == NULL || size == 0 || arena->first_mempool == NULL) { goto arena_handle_err; }

	const size_t input_bytes = mempool_add_padding(size);
	if (input_bytes != size)
	{
		printf("debug: padding has been added. input: %lu, padded: %lu\n", size, input_bytes);
		fflush(stdout);
	}
	Mempool_Header *head = NULL;

	head = mempool_find_block(arena, input_bytes);
	if (!head) { goto arena_handle_err; }
	user_handle = mempool_create_handle_and_entry(arena, head);
	if (user_handle.addr != NULL) { return user_handle; }

arena_handle_err:
	perror("error: invalid handle was given! things will break!\n");
	fflush(stdout);
	return user_handle;
}


void
arena_reset(const Arena *restrict arena, const int reset_type)
{
	Mempool *restrict pool = arena->first_mempool;

	switch (reset_type)
	{
		case 1:
		mp_soft_reset:
			pool->mem_offset = 0;
			return;
		case 0:
			pool->mem_offset = 0;
		case 2:
			if (!pool->next_pool) { return; }

			while (pool->next_pool)
			{
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->mem_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = NULL;
				pool->prev_pool = NULL;
			}
			arena->first_mempool->mem = (char *)0;
			break;
		default:
			goto mp_soft_reset;
	}
}


void
arena_free(Arena_Handle *user_handle)
{
	Mempool_Header *head = user_handle->header;

	Arena *arena = return_base_arena(user_handle);

	if (!mempool_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return;
	}

	user_handle->header->flags = 1;
	user_handle->generation++;
	user_handle->addr = NULL;

	arena_defragment(arena, 0);
}


int
arena_realloc(Arena_Handle *user_handle, size_t size)
{
	// it's probably best to memcpy here to allow reallocation, and track
	// the address by updating the handle if it is not locked.
	if (user_handle->header->flags == FROZEN || user_handle == NULL) { return 1; }
	size = mempool_add_padding(size);

	Mempool_Header *old_head = user_handle->header;
	Arena *arena = return_base_arena(user_handle);
	Mempool_Header *new_head = mempool_find_block(arena, size);
	if (!new_head) { return 1; }

	void *old_block = (char *)old_head + PD_HEAD_SIZE;
	void *new_block = (char *)new_head + PD_HEAD_SIZE;
	if (!old_block || !new_block) { return 1; }
	/* if there is a new head and nothing is null then it's all good to go for reallocation. */

	memcpy(new_block, old_block, old_head->chunk_size - PD_HEAD_SIZE);
	user_handle->generation++;
	user_handle->header = new_head;
	user_handle->addr = new_block;

	old_head->flags = FREE;

	return 0;
}


void *
handle_lock(Arena_Handle *restrict user_handle)
{
	const Arena *arena = return_base_arena(user_handle);

	if (!mempool_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return NULL;
	}
	user_handle->generation++;

	user_handle->header->flags = FROZEN;

	return (user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE));
}


void
handle_unlock(Arena_Handle *user_handle)
{
	Arena *arena = return_base_arena(user_handle);
	mempool_update_table_generation(user_handle);

	if (!mempool_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected! not continuing!\n");
		return;
	}
	user_handle->header->flags = ALLOCATED;
	user_handle->addr = NULL;
	arena_defragment(arena, LIGHT_DEFRAG);
}


void
arena_debug_print_memory_usage(const Arena *arena)
{
	const Mempool *mempool = arena->first_mempool;
	const Mempool_Header *header = (Mempool_Header *)arena->first_mempool->mem;

	u_int32_t pool_count = 0, head_count = 0;
	size_t total_pool_mem = mempool->mem_size;

	while (mempool)
	{
		total_pool_mem += mempool->mem_size;
		mempool = mempool->next_pool;
		pool_count++;
	}
	while (header)
	{
		header = header->next_header;
		head_count++;
	}

	printf("Total amount of pools: %d\n", pool_count);
	printf("Pool memory (B): %lu\n", total_pool_mem);

	const size_t header_bytes = head_count * PD_HEAD_SIZE;
	size_t handle_count = 0;

	for (size_t i = 0; i < arena->table_count; i++) { handle_count += arena->handle_table[i]->entries; }

	const size_t handle_bytes = handle_count * PD_HANDLE_SIZE;
	const size_t reserved_mem = handle_bytes + header_bytes + (pool_count * PD_POOL_SIZE) + PD_ARENA_SIZE;

	printf("Struct memory of: headers: %lu\n", header_bytes);
	printf("Struct memory of: handles: %lu\n", handle_bytes);
	printf("Struct memory of: pools: %lu\n", pool_count * PD_POOL_SIZE);
	printf("Total reserved memory: %lu\n", reserved_mem);
	printf("Total memory used: %lu\n", total_pool_mem + reserved_mem);
	fflush(stdout);
}