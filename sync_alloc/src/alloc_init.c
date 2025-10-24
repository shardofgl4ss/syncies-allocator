//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppDeclaratorNeverUsed

#include "alloc_init.h"
#include "handle.h"
#include "helper_functions.h"

Arena *
arena_init()
{
	void *raw_pool = mp_helper_map_mem(
		(MAX_FIRST_POOL_SIZE + PD_RESERVED_F_SIZE) + (ALIGNMENT - 1) & (usize)~(ALIGNMENT - 1)
	);

	if (raw_pool == MAP_FAILED)
		goto alloc_failure;

	Arena *arena = raw_pool;

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set mem_size to the FIRST_POOL_ALLOC.					*
	 *	This spoofs a max memory of POOL_SIZE when its actually POOL_SIZE + reserved.				*
	 *  Alignment is added to Mempool in case the user changes it to 16 or 32, it will be aligned.	*
	 *  The alignment however adds some complexity later on.										*/

	auto *first_pool = (Memory_Pool *)((char *)raw_pool + PD_ARENA_SIZE);

	first_pool->mem = (void *)((char *)raw_pool + PD_POOL_SIZE);
	first_pool->pool_offset = 0;
	first_pool->pool_size = MAX_FIRST_POOL_SIZE;
	first_pool->next_pool = nullptr;
	first_pool->prev_pool = nullptr;

	arena->total_mem_size = MAX_FIRST_POOL_SIZE;
	arena->table_count = 0;
	arena->first_hdl_tbl = mempool_new_handle_table(arena, arena->first_hdl_tbl);

	return arena;

alloc_failure:
	return nullptr;
}

Memory_Pool *
pool_init(Arena *arena, const u32 size)
{
	Memory_Pool *pool = arena != nullptr ? arena->first_mempool : nullptr;

	if (pool == nullptr)
		goto mp_internal_error;
	while (pool->next_pool != nullptr)
		pool = pool->next_pool;

	void *raw = mp_helper_map_mem(size + PD_POOL_SIZE);

	if (raw == nullptr)
		goto mp_internal_error;

	Memory_Pool *new_pool = raw;
	new_pool->mem = (void *)((char *)raw + PD_POOL_SIZE);

	pool->next_pool = new_pool;
	new_pool->prev_pool = pool;

	new_pool->pool_size = size;
	new_pool->pool_offset = 0;
	new_pool->free_count = 0;
	new_pool->next_pool = nullptr;
	new_pool->first_free = nullptr;

	arena->total_mem_size += new_pool->pool_size;
	Pool_Header *sentinel_header = new_pool->mem + (size - PD_HEAD_SIZE);

	sentinel_header->handle_idx = 0;
	sentinel_header->size = 0;
	sentinel_header->prev_block_size = 0;
	sentinel_header->block_flags |= PH_SENTINEL | PH_FROZEN;

	return new_pool;

mp_internal_error:
	sync_alloc_log.to_console(log_stderr, "error: failed to create internal memory pool!\n");
	fflush(stdout);
	return nullptr;
}
