//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "debug.h"
#include "helper_functions.h"


Pool_Header *
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
	sync_alloc_log.to_console(log_stderr, "error: not enough room in pool for header!\n");
	return nullptr;
}

Pool_Header *
mempool_find_block(const Arena *restrict arena, const u16 requested_size)
{
	// TODO make mempool_find_block() work

	if (arena == nullptr
	    || arena->first_mempool == nullptr
	    || requested_size == 0
	) { goto fail_nullptr; }

	const Memory_Pool *pool = arena->first_mempool;

	while (pool->first_free_offset == 0) {
			if (pool->next_pool == nullptr)
				goto fail_no_pool;
			pool = pool->next_pool;
	}

	u32 free_offset = pool->first_free_offset;
	Pool_Free_Header *free_header = nullptr;
	Pool_Header *new_head = nullptr;

	do {


	}
	while (new_head == nullptr)
	// do {
	// 	free_header = (Pool_Free_Header *)((char *)pool->mem + free_offset);
	// 	new_head = (free_header->size >= requested_size + PD_HEAD_SIZE)
	// 	           ? mempool_create_header(pool, requested_size, free_offset)
	// 	           : nullptr;
	// 	if (new_head != nullptr)
	// 		goto block_found;
	// 	free_offset = free_header->next_free_offset;
	// 	if (free_offset == 0) {
	// 		do {
	// 			if (pool->next_pool == nullptr)
	// 				goto fail_no_pool;
	// 			pool = pool->next_pool;
	// 		}
	// 		while (pool->free_count == 0);
	// 	}
	// }
	// while (new_head == nullptr);
block_found:
	return new_head;
fail_nullptr:
fail_no_pool:
	sync_alloc_log.to_console(log_stderr, "error: new block could not be found!\n");
	return nullptr;
}
