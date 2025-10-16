//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "alloc_lib.h"

static Pool_Header *
mempool_create_header(const Memory_Pool *restrict pool, const u16 size, const u32 offset)
{
	if ((pool->pool_size - pool->pool_offset) < (mp_helper_add_padding(size) + PD_HEAD_SIZE))
		goto mp_head_create_error;

	auto *head = (Pool_Header *)((char *)pool->mem + offset);

	head->size = (u16)((size + PD_HEAD_SIZE) << 3);
	head->size = (head->size & ~0x7) | ((u8)ALLOCATED & 0x7);
	head->handle_idx = 0;
	head->prev_block_size = 0;

	return head;

mp_head_create_error:
	printf("error: not enough room in pool for header!\n");
	fflush(stdout);
	return nullptr;
}


static Memory_Pool *
mempool_create_internal_pool(Arena *restrict arena, const u32 size)
{
	Memory_Pool *pool = arena != nullptr ? arena->first_mempool : nullptr;

	if (pool == nullptr)
		goto mp_internal_error;
	if (pool->next_pool)
		while (pool->next_pool) pool = pool->next_pool;

	void *raw = mp_helper_map_mem(size + PD_POOL_SIZE);

	if (raw == nullptr)
		goto mp_internal_error;

	Memory_Pool *new_pool = raw;
	new_pool->mem = (void *)((char *)raw + PD_POOL_SIZE);

	pool->next_pool = new_pool;
	new_pool->prev_pool = pool;

	new_pool->pool_size = size;
	new_pool->pool_offset = 0;
	new_pool->first_free_offset = 0;
	new_pool->free_count = 0;
	new_pool->next_pool = nullptr;

	arena->total_mem_size += new_pool->pool_size;
	Pool_Header *sentinel_header = new_pool->mem + (size - PD_HEAD_SIZE);

	sentinel_header->handle_idx = 0;
	sentinel_header->size = 0;
	sentinel_header->prev_block_size = 0;

	return new_pool;

mp_internal_error:
	perror("error: failed to create internal memory pool!\n");
	fflush(stdout);
	return nullptr;
}


static Pool_Header *
mempool_find_block(const Arena *restrict arena, const u16 requested_size)
{
	if (arena == nullptr
	    || arena->first_mempool == nullptr
	    || requested_size == 0
	) { goto fail; }

	u32 user_size = requested_size;
	Memory_Pool *pool = arena->first_mempool;
	u32 free_offset = pool->first_free_offset;

	// The first pool that has free blocks is selected. First match wins for speed.
	if (free_offset == 0)
	{
		while (pool->next_pool != nullptr)
		{
			pool = pool->next_pool;
			if (pool->first_free_offset != 0)
			{
				free_offset = pool->first_free_offset;
				goto red_core_loaded;
			}
		}
		goto fail;
	}

red_core_loaded:
	// fix later

	const Pool_Free_Header *free_header = (Pool_Free_Header *)((char *)pool->mem + free_offset);
	free_offset = (u32)((uintptr)pool->mem - (uintptr)free_header);

	bool valid_next = free_header->next_free_offset != 0 ? true : false;

	Pool_Header *new_head = (free_header->size > requested_size + PD_HEAD_SIZE)
	                        ? mempool_create_header(pool, requested_size, free_offset)
	                        : nullptr;
	if (new_head != nullptr)
		return new_head;

rerun_free_list:
	while (valid_next)
	{
		free_header = (Pool_Free_Header *)((char *)pool->mem + free_offset);
		valid_next = free_header->next_free_offset != 0 ? true : false;

		new_head = (free_header->size > requested_size + PD_HEAD_SIZE)
		           ? mempool_create_header(pool, requested_size, free_offset)
		           : nullptr;

		if (new_head != nullptr)
			return new_head;
		free_offset = free_header->next_free_offset;
	}
	while (pool->free_count == 0)
	{
		if (pool->next_pool == nullptr)
			goto fail;
		pool = pool->next_pool;
	}
	free_offset = pool->first_free_offset;
	goto rerun_free_list;

fail:
	perror("error: new block could not be found!\n");
	fflush_unlocked(stdout);
	return nullptr;
}