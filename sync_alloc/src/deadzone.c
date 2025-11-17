//
// Created by SyncShard on 11/17/25.
//

#include "deadzone.h"
#include "defs.h"
#include "globals.h"
#include "structs.h"
#include "types.h"

static constexpr u32 PREV_SIZE_OFFSET = sizeof(int) * 3;
static constexpr u32 QUARTER_SIZE = DEADZONE_PADDING / 2;


inline bool corrupt_header_check(pool_header_t *restrict head)
{
	return *(u32 *)((char *)head + (head->chunk_size - (DEADZONE_PADDING * 2))) != HEAD_DEADZONE;
}


inline bool corrupt_pool_check(memory_pool_t *pool)
{
	return (*(u64 *)((char *)pool + PD_POOL_SIZE) != POOL_DEADZONE);
}


inline u32 return_prev_block_size(pool_header_t *head)
{
	if (head->bitflags & F_FIRST_HEAD) {
		return *(u32 *)(head - 1);
	}
	return *(u32 *)((char *)(head - 1) + PREV_SIZE_OFFSET);
}


inline int create_head_deadzone(pool_header_t *head, memory_pool_t *pool)
{
	if (!head || !pool) {
		return 1;
	}
	const char *const base_zone = (char *)head + (head->chunk_size - ALIGNMENT);

	u32 *deadzone = (u32 *)(base_zone);
	memory_pool_t **pool_ptr_zone = (memory_pool_t **)(base_zone + QUARTER_SIZE);
	u32 *prev_size_zone = (u32 *)(base_zone + (PREV_SIZE_OFFSET));

	/* DEADZONE (4)--POOL_PTR (8)--PREV_SIZE (4) */

	*deadzone = HEAD_DEADZONE;
	*pool_ptr_zone = pool;
	*prev_size_zone = head->chunk_size;
	return 0;
}


inline int create_pool_deadzone(void *heap_start)
{
	if (!heap_start) {
		return 1;
	}

	// This might need to be changed for certain architectures, since its 8 byte aligned before a 64 alignment,
	// making it appear at: 00 00 00 00 <ad de ad de>, if ARM requires a double-quadword r/w alignment, itll have
	// to be pushed back by 8 bytes. Itll be fine if it just needs quadword (or less) alignment though.

	u64 *deadzone = (u64 *)((char *)heap_start - DEADZONE_PADDING);
	*deadzone = POOL_DEADZONE;
	memory_pool_t **pool_ptr_deadzone = (memory_pool_t **)(deadzone - 1);
	*pool_ptr_deadzone = heap_start;

	return 0;
}