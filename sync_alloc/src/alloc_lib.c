//
// Created by SyncShard on 10/9/25.
//

#include "alloc_lib.h"
#include <string.h>
#include "internal_alloc.h"


extern Arena *
arena_create()
{
	void *raw_pool = mp_helper_map_mem(
		(MAX_FIRST_POOL_SIZE + PD_RESERVED_F_SIZE) + (ALIGNMENT - 1) & (usize)~(ALIGNMENT - 1)
	);

	if (raw_pool == MAP_FAILED)
		goto mp_create_error;

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set mem_size to the FIRST_POOL_ALLOC.					*
	 *	This spoofs a max memory of POOL_SIZE when its actually POOL_SIZE + reserved.				*
	 *  Alignment is added to Mempool in case the user changes it to 16 or 32, it will be aligned.	*
	 *  The alignment however adds some complexity later on.										*/

	Arena *arena = raw_pool;
	auto *first_pool = (Memory_Pool *)((char *)raw_pool + PD_ARENA_SIZE);

	first_pool->mem = (void *)((char *)raw_pool + PD_POOL_SIZE);
	first_pool->pool_offset = 0;
	first_pool->pool_size = MAX_FIRST_POOL_SIZE;
	first_pool->next_pool = nullptr;
	first_pool->prev_pool = nullptr;

	arena->total_mem_size = MAX_FIRST_POOL_SIZE;
	arena->table_count = 0;
	arena->first_hdl_tbl = mempool_new_handle_table(arena);

	return arena;

mp_create_error:
	perror("ERR_NO_MEMORY: arena creation failed!\n");
	fflush(stdout);
	return nullptr;
}


extern void
arena_destroy(const Arena *const restrict arena)
{
	if (arena == nullptr || arena->first_mempool == nullptr)
		return;
	const Memory_Pool *pool = arena->first_mempool;

	if (pool->next_pool == nullptr)
	{
		mp_helper_destroy(pool->mem, pool->pool_size + PD_RESERVED_F_SIZE);
		printf("arena destroyed\n");
		fflush(stdout);
	}

	pool = pool->next_pool;
	mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_RESERVED_F_SIZE);

	while (pool->next_pool != nullptr)
	{
		pool = pool->next_pool;
		mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
	}

	mp_helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);

	printf("arena destroyed\n");
	fflush(stdout);
}


extern void
arena_defragment(const Arena *const restrict arena, const bool l_defrag)
{
	const Memory_Pool *restrict pool = (arena) ? arena->first_mempool : nullptr;
	const Pool_Header *restrict head = (pool->pool_offset != 0) ? pool->mem : nullptr;

	if (pool == nullptr
	    || head == nullptr
	    || pool->pool_size == 0
	    || (
		    pool->pool_offset == 0
		    && pool->next_pool == nullptr
	    )
	) { return; }

	if (!l_defrag)
		goto heavy_defrag;

light_defrag:
	u_int32_t pool_idx = 0;

	while (pool != nullptr)
	{
		if (pool->pool_offset == 0)
			goto empty_pool;

		//const auto *head = (Mempool_Header *)((char *)pool->mem);
		Pool_Header *prev_head = head->prev_header;
		Pool_Header *next_head = head->next_header;

		while (head != nullptr && head->flags == FREE)
		{
			if (head->pool_id != pool_idx)
			{
				fprintf(
					stderr,
					"error: pool IDs do not match! current head: %d, current pool: %d, skipping...\n",
					head->pool_id,
					pool_idx
				);
				//head->pool_id = pool_idx;
				continue;
			}

			if (prev_head != nullptr
			    && prev_head->flags == FREE
			    && prev_head->pool_id == head->pool_id
			)
			{
				prev_head->block_size += head->block_size + PD_HEAD_SIZE;
				prev_head->next_header = next_head;
				next_head->prev_header = prev_head;
			}

			head = head->next_header;
		}
		pool = pool->next_pool, pool_idx++;
		continue;

	empty_pool:
		if (pool->prev_pool == nullptr)
			continue;
		mp_helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);
	}
	return;

heavy_defrag:
	// wip, so just make it go to light defrag for now
	goto light_defrag;
}


extern Arena_Handle
arena_alloc(Arena *arena, const size_t size)
{
	Arena_Handle user_handle = {
		.addr = nullptr,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = nullptr,
	};

	if (arena == nullptr || size == 0 || arena->first_mempool == nullptr) { return user_handle; }

	const size_t input_bytes = mp_helper_add_padding(size);
	if (input_bytes != size)
	{
		printf("debug: padding has been added. input: %lu, padded: %lu\n", size, input_bytes);
		fflush(stdout);
	}
	Pool_Header *head = nullptr;

	head = mempool_find_block(arena, input_bytes);
	if (head == nullptr)
		perror("invalid handle error");
	user_handle = mempool_create_handle_and_entry(arena, head);
	return user_handle;
}


extern void
arena_reset(const Arena *restrict arena, const int reset_type)
{
	Memory_Pool *restrict pool = arena->first_mempool;

	switch (reset_type)
	{
		default:
		case 1:
			pool->pool_offset = 0;
			return;
		case 0:
			pool->pool_offset = 0;
		case 2:
			if (!pool->next_pool) { return; }

			while (pool->next_pool)
			{
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = nullptr;
				pool->prev_pool = nullptr;
			};
			break;
	}
}


extern void
arena_free(Arena_Handle *user_handle)
{
	Arena *arena = mp_helper_return_base_arena(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return;
	}

	user_handle->header->flags = FREE;
	user_handle->generation++;
	user_handle->addr = nullptr;

	arena_defragment(arena, 0);
}


extern int
arena_realloc(Arena_Handle *user_handle, size_t size)
{
	// it's probably best to memcpy here to allow reallocation, and track
	// the address by updating the handle if it is not locked.
	if (user_handle->header->flags == FROZEN || user_handle == nullptr) { return 1; }
	size = mp_helper_add_padding(size);

	Pool_Header *old_head = user_handle->header;
	Arena *arena = mp_helper_return_base_arena(user_handle);
	Pool_Header *new_head = mempool_find_block(arena, size);
	if (!new_head) { return 1; }

	void *old_block = (char *)old_head + PD_HEAD_SIZE;
	void *new_block = (char *)new_head + PD_HEAD_SIZE;
	if (!old_block || !new_block) { return 1; }
	/* if there is a new head and nothing is null then it's all good to go for reallocation. */

	memcpy(new_block, old_block, old_head->block_size);
	user_handle->generation++;
	user_handle->header = new_head;
	user_handle->addr = new_block;

	old_head->flags = FREE;

	return 0;
}


extern void *
handle_lock(Arena_Handle *restrict user_handle)
{
	const Arena *arena = mp_helper_return_base_arena(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return nullptr;
	}
	user_handle->generation++;

	user_handle->header->flags = FROZEN;

	return (user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE));
}


extern void
handle_unlock(Arena_Handle *user_handle)
{
	const Arena *arena = mp_helper_return_base_arena(user_handle);
	mp_helper_update_table_generation(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle))
	{
		perror("error: stale handle detected! not continuing!\n");
		return;
	}
	user_handle->header->flags = ALLOCATED;
	user_handle->addr = nullptr;
	arena_defragment(arena, true);
}


extern void
arena_debug_print_memory_usage(const Arena *arena)
{
	const Memory_Pool *mempool = arena->first_mempool;
	const Pool_Header *header = (Pool_Header *)arena->first_mempool->mem;

	u_int32_t pool_count = 0, head_count = 0;
	size_t total_pool_mem = mempool->pool_size;

	while (mempool)
	{
		total_pool_mem += mempool->pool_size;
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

	for (size_t i = 0; i < arena->table_count; i++) { handle_count += arena->first_hdl_tbl[i]->entries_bitmap; }

	const size_t handle_bytes = handle_count * PD_HANDLE_SIZE;
	const size_t reserved_mem = handle_bytes + header_bytes + (pool_count * PD_POOL_SIZE) + PD_ARENA_SIZE;

	printf("Struct memory of: headers: %lu\n", header_bytes);
	printf("Struct memory of: handles: %lu\n", handle_bytes);
	printf("Struct memory of: pools: %lu\n", pool_count * PD_POOL_SIZE);
	printf("Total reserved memory: %lu\n", reserved_mem);
	printf("Total memory used: %lu\n", total_pool_mem + reserved_mem);
	fflush(stdout);
}