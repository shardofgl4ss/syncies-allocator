//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#if ALLOC_DEBUG_LVL != 0
#include "debug.h"
#endif
#include "helper_functions.h"


Pool_Header *
mempool_create_header(const Memory_Pool *restrict pool, const u32 size, const intptr offset)
{
	// TODO make mempool_create_header take in a bitmap argument for flags and fix all the calls
	if ((pool->pool_size - pool->pool_offset) < (mp_helper_add_padding(size) + PD_HEAD_SIZE))
		goto mp_head_create_error;

	auto *head = (Pool_Header *)((char *)pool->mem + offset);

	head->size = size + PD_HEAD_SIZE;
	head->handle_idx = 0;
	head->prev_block_size = 0;
	head->block_flags = 0;
	head->block_flags |= PH_ALLOCATED;

	return head;

mp_head_create_error:
	#if ALLOC_DEBUG_LVL != 0
	sync_alloc_log.to_console(log_stderr, "error: not enough room in pool for header!\n");
	#endif
	return nullptr;
}

Pool_Header *
mempool_find_block(const Arena *restrict arena, const u32 requested_size)
{
	// I hope this works

	// TODO make sure mempool_find_block can create its own headers, not just on top of free ones
	// (ie, when offset is zero or size + PD_HEAD_SIZE < max_pool_size

	if (arena == nullptr
	    || arena->first_mempool == nullptr
	    || requested_size == 0
	) { goto fail_nullptr; }

	const Memory_Pool *pool = arena->first_mempool;

	while (pool->first_free == nullptr) {
		if (pool->next_pool == nullptr)
			goto fail_no_pool;
		pool = pool->next_pool;
	}

	Pool_Free_Header *free_header = pool->first_free;
	Pool_Header *new_head;
	intptr relative_offset = 0;

	do {
		if (free_header->block_flags & PH_SENSITIVE) {
			memset(free_header + PD_FREE_PH_SIZE, 0, free_header->size - PD_FREE_PH_SIZE);
			free_header->block_flags &= ~PH_SENSITIVE;
		}
		relative_offset = (intptr)free_header - (intptr)pool->mem;
		new_head = (free_header->size >= requested_size + PD_HEAD_SIZE)
		           ? mempool_create_header(pool, requested_size, relative_offset)
		           : nullptr;
		if (new_head != nullptr)
			break;
		if (free_header->next_free == nullptr) {
			while (pool->next_pool != nullptr) {
				pool = pool->next_pool;
				if (pool->first_free != nullptr) {
					free_header = pool->first_free;
					break;
				}
				if (pool->next_pool == nullptr)
					goto fail_no_pool;
			}
		}
		free_header = free_header->next_free;
	}
	while (new_head == nullptr);

	return new_head;
fail_nullptr:
	#if ALLOC_DEBUG_LVL != 0
	sync_alloc_log.to_console(log_stderr, "mempool_find_block args were null!\n");
	#endif
fail_no_pool:
	#if ALLOC_DEBUG_LVL != 0
	sync_alloc_log.to_console(log_stderr, "new block could not be found!\n");
	#endif
	return nullptr;
}
