//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "debug.h"
#include "helper_functions.h"

extern _Thread_local Arena *arena_thread;

Pool_Header *
mempool_create_header(Memory_Pool *restrict pool, const u32 size, const intptr offset)
{
	if ((pool->pool_size - pool->pool_offset) < (helper_add_padding(size) + PD_HEAD_SIZE))
		goto mp_head_create_error;

	// just for show, the way this is added is actually the memory layout of each header-alloc-pad.
	const u32 chunk_size = (u32)helper_add_padding(PD_HEAD_SIZE + size + DEADZONE_PADDING);
	auto *head = (Pool_Header *)((char *)pool->mem + offset);

	head->size = chunk_size;
	head->handle_idx = 0;
	head->prev_block_size = 0;
	head->block_flags |= PH_ALLOCATED;

	auto *deadzone = (u64 *)((char *)head + chunk_size - DEADZONE_PADDING);
	*deadzone = HEAP_DEADZONE;

	pool->pool_offset += chunk_size;
	return head;

mp_head_create_error:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "not enough room in pool for header!\n");
	#endif
	return nullptr;
}

Pool_Header *
mempool_find_block(const u32 requested_size)
{	// I hope this works
	if (arena_thread == nullptr
	    || arena_thread->first_mempool == nullptr
	    || requested_size == 0)
		goto fail_nullptr;

	Memory_Pool *pool = arena_thread->first_mempool;

	Pool_Header *new_head;

	// if the quickest options for just checking offset and if it can fit in remaining space
	// return successfully, then we just use those. Only when pools are full do we walk free lists.
	// cause free list walking is (probably) slower than just immediately knowing you can put one.

	if (pool->pool_offset == 0) {
		new_head = mempool_create_header(pool, requested_size, 0);
		if (new_head != nullptr) {
			new_head->block_flags |= PH_SENTINEL_F;
			goto found_block;
		}
	}
	if ((pool->pool_offset + requested_size + PD_HEAD_SIZE) < pool->pool_size) {
		new_head = mempool_create_header(pool, requested_size, pool->pool_offset);
		if (new_head != nullptr) {
			new_head->block_flags |= PH_SENTINEL_L;
			Pool_Header *prev = new_head - new_head->size;
			if (prev != nullptr && prev->block_flags & PH_SENTINEL_L)
				prev->block_flags &= ~PH_SENTINEL_L;
			goto found_block;
		}
	}
	// if those checks don't work, now walk the free list
	while (pool->first_free == nullptr) {
		if (pool->next_pool == nullptr)
			goto fail_no_pool;
		pool = pool->next_pool;
	}

	Pool_Free_Header *free_header = pool->first_free;
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
			goto found_block;
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

found_block:
	return new_head;
fail_nullptr:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "mempool_find_block args were null!\n");
	#endif
fail_no_pool:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "new block could not be found!\n");
	#endif
	return nullptr;
}
