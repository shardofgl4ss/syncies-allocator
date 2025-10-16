//
// Created by SyncShard on 10/9/25.
//

#include "alloc_lib.h"
#include <stdio.h>
#include <string.h>
#include "alloc_init.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"


Arena *
arena_create()
{
	debug_init();
	Arena *arena = arena_init();
	if (arena != nullptr) {
		logger.to_console("Arena creation successful!", 0);
		return arena;
	}

	logger.to_console(
		"ERR_NO_MEMORY: arena allocation failure! When you buy more ram, send some to your local protogen too!",
		true
	);
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
		printf("arena destroyed\n");
		fflush(stdout);
	}

	pool = pool->next_pool;
	mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_RESERVED_F_SIZE);

	while (pool->next_pool != nullptr) {
		pool = pool->next_pool;
		mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
	}

	mp_helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);

	printf("arena destroyed\n");
	fflush(stdout);
}


void
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

	while (pool != nullptr) {
		if (pool->pool_offset == 0)
			goto empty_pool;

		//const auto *head = (Mempool_Header *)((char *)pool->mem);
		Pool_Header *prev_head = head->prev_header;
		Pool_Header *next_head = head->next_header;

		while (head != nullptr && head->flags == FREE) {
			if (head->pool_id != pool_idx) {
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
			) {
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


ATTR_ALLOC_SIZE(2) Arena_Handle
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
	if (input_bytes != size) {
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

	user_handle->header->flags = FREE;
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

	old_head->flags = FREE;

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