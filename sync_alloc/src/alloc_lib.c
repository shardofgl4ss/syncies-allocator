//
// Created by SyncShard on 10/9/25.
//

#include "alloc_lib.h"
#include "alloc_init.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include <stdio.h>
#include <string.h>


Arena *
arena_create()
{
	Arena *arena = arena_init();
	if (arena == nullptr)
		goto arena_fail;

	#if ALLOC_DEBUG_LVL != 0
	sync_alloc_log.to_console(log_stdout, "Arena creation successful!");
	#endif

	return arena;

arena_fail:
	#if ALLOC_DEBUG_LVL != 0
	sync_alloc_log.to_console(
		log_stdout,
		"arena allocation failure! When you buy more ram, send some to your local protogen too!"
	);
	#endif
	return nullptr;
}


void
arena_destroy(const Arena *const restrict arena)
{
	if (arena == nullptr || arena->first_mempool == nullptr)
		return;
	const Memory_Pool *pool = arena->first_mempool;

	if (pool->next_pool == nullptr) {
		mp_helper_destroy(pool->mem, pool->pool_size + PD_RESERVED_F_SIZE);
		sync_alloc_log.to_console(log_stdout, "arena destroyed\n");
	}

	pool = pool->next_pool;
	mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_RESERVED_F_SIZE);

	while (pool->next_pool != nullptr) {
		pool = pool->next_pool;
		mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
	}

	mp_helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);

	sync_alloc_log.to_console(log_stdout, "arena destroyed\n");
}


void
arena_defragment(const Arena *const restrict arena, const bool l_defrag)
{
	Memory_Pool *restrict pool = (arena) ? arena->first_mempool : nullptr;
	Pool_Header *restrict head = (pool->pool_offset != 0) ? pool->mem : nullptr;

	if (pool == nullptr
	    || head == nullptr
	    || pool->pool_size == 0
	    || (
		    pool->pool_offset == 0
		    && pool->next_pool == nullptr
	    )
	) { return; }
}


Arena_Handle
arena_alloc(Arena *arena, const usize size)
{

	Arena_Handle user_handle = {
		.addr = nullptr,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = nullptr,
	};

	if (arena == nullptr || size == 0 || arena->first_mempool == nullptr) { return user_handle; }

	Memory_Pool *pool = arena->first_mempool;

	// 4 million is the unsigned 32 bit limit, so we can just use 4mb
	if (MEBIBYTE * 4 < size) // return user handle for now as large page allocation isn't done yet.
		return user_handle;

	const u32 input_bytes = (u32)mp_helper_add_padding(size);

	#if ALLOC_DEBUG_LVL != 0
	if (input_bytes != size) {
		sync_alloc_log.to_console(log_stdout, "debug: padding has been added. input: %lu, padded: %lu\n", size, input_bytes);
	}
	#endif


	Pool_Header *head = nullptr;
	do {
		// TODO make sure mempool_find_block can create its own headers, not just on top of free ones
		// (ie, when offset is zero)

		head = mempool_find_block(arena, input_bytes);
		if (head == nullptr) {
			while (pool->next_pool != nullptr)
				pool = pool->next_pool;

			Memory_Pool *new_pool = pool_init(arena, pool->pool_size * 2);
			if (new_pool == nullptr)
				goto memory_error;
			pool->next_pool = new_pool;
		}
	}
	while (head == nullptr);

	user_handle = mempool_create_handle_and_entry(arena, head);
	return user_handle;

	memory_error:
	sync_alloc_log.to_console(log_stderr, "out of memory error, but trying to continue!\n");
	return user_handle;
}


void
arena_reset(const Arena *restrict arena, const int reset_type)
{
	Memory_Pool *restrict pool = arena->first_mempool;

	switch (reset_type) {
		default:
		case 1:
			pool->pool_offset = 0;
			return;
		case 0:
			pool->pool_offset = 0;
		case 2:
			if (!pool->next_pool) { return; }

			while (pool->next_pool) {
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = nullptr;
				pool->prev_pool = nullptr;
			};
			break;
	}
}


void
arena_free(Arena_Handle *user_handle)
{
	Arena *arena = mp_helper_return_base_arena(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle)) {
		perror("error: stale handle detected!\n");
		return;
	}

	user_handle->header->flags = H_FREE;
	user_handle->generation++;
	user_handle->addr = nullptr;

	arena_defragment(arena, 0);
}


int
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

	old_head->flags = H_FREE;

	return 0;
}


void *
handle_lock(Arena_Handle *restrict user_handle)
{
	const Arena *arena = mp_helper_return_base_arena(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle)) {
		perror("error: stale handle detected!\n");
		return nullptr;
	}
	user_handle->generation++;

	user_handle->header->flags = FROZEN;

	return (user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE));
}


void
handle_unlock(Arena_Handle *user_handle)
{
	const Arena *arena = mp_helper_return_base_arena(user_handle);
	mp_helper_update_table_generation(user_handle);

	if (!mp_helper_handle_generation_checksum(arena, user_handle)) {
		perror("error: stale handle detected! not continuing!\n");
		return;
	}
	user_handle->header->flags = ALLOCATED;
	user_handle->addr = nullptr;
	arena_defragment(arena, true);
}
