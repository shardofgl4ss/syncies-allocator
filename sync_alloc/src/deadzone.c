//
// Created by SyncShard on 11/17/25.
//

#include "deadzone.h"
#include "globals.h"
#include "structs.h"
#include "syn_memops.h"
#include "types.h"


inline bool corrupt_header_check(pool_header_t *restrict head)
{
	const head_deadzone_t *head_dz =
		(head_deadzone_t *)((char *)head + (head->chunk_size - DEADZONE_SIZE));
	return (head_dz->deadzone != HEAD_DEADZONE);
}


inline bool corrupt_pool_check(const memory_pool_t *pool)
{
	const pool_deadzone_t *pool_dz = (pool_deadzone_t *)((char *)pool->mem - DEADZONE_SIZE);
	return (pool_dz->deadzone != POOL_DEADZONE);
}


inline u32 return_prev_block_size(pool_header_t *head)
{
	return (head->bitflags & F_FIRST_HEAD) ? 0
	                                       : ((head_deadzone_t *)(head - 1))->prev_chunk_size;
}


inline int create_head_deadzone(const pool_header_t *head, const memory_pool_t *pool)
{
	if (!head || !pool) {
		return 1;
	}
	void *head_dz = (head_deadzone_t *)((char *)head + (head->chunk_size - DEADZONE_SIZE));

	const head_deadzone_t deadzone = {
		.deadzone = HEAD_DEADZONE,
		.prev_chunk_size = head->chunk_size,
		.pool_ptr = pool,
	};

	syn_memcpy(head_dz, &deadzone, DEADZONE_SIZE);
	return 0;
}


inline int create_pool_deadzone(const memory_pool_t *pool)
{
	if (!pool) {
		return 1;
	}

	void *pool_dz = ((char *)pool->mem - DEADZONE_SIZE);

	const pool_deadzone_t deadzone = {
		.pool_ptr = pool,
		.deadzone = POOL_DEADZONE,
	};

	syn_memcpy(pool_dz, &deadzone, DEADZONE_SIZE);
	return 0;
}
