//
// Created by SyncShard on 10/26/25.
//

#include "alloc_utils.h"
#include "alloc_init.h"
#include "debug.h"
#include "defs.h"
#include "free_node.h"
#include "handle.h"
#include "structs.h"
#include "types.h"

// Not implemented
// void
// defragment_pool(Memory_Pool *pool);

#ifdef SYN_USE_RAW

inline int bad_alloc_check(const void *, const int do_checksum)
{
}

#else

inline int bad_alloc_check(const syn_handle_t *restrict hdl, const int do_checksum)
{
	if (hdl == nullptr || hdl->header == nullptr) {
		return -1;
	}
	if (arena_thread == nullptr) {
		syn_panic("core arena context was lost!\n");
	}
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (hdl->header->bitflags & F_SENTINEL) {
		goto skip_header;
	}
	if (corrupt_header_check(hdl->header)) {
		syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	}
	#endif
skip_header:
	if (do_checksum && !handle_generation_checksum(hdl)) {
		sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
		return 1;
	}
	if (hdl->header->bitflags & F_FROZEN) {
		return 2;
	}
	return 0;
}


memory_pool_t *return_pool(pool_header_t *header)
{
	// cursed $!# pointer arithmetic and casting so bad I prolly broke the ABI
	return *(memory_pool_t **)(((char *)header + header->chunk_size) - ((DEADZONE_PADDING / 2) * 3));
	// 	pool_header_t *current_head = header;
	// reloop:
	// 	if (current_head->bitflags & F_FIRST_HEAD) {
	// 		return ((memory_pool_t *)((char *)current_head - PD_RESERVED_F_SIZE));
	// 	}
	//
	// 	// If this goes past the boundary and out of bounds, it should terminate instead of infinitely looping.
	// 	const u32 deadzone_prev_size = *((u32 *)(((char *)current_head + current_head->chunk_size) - (DEADZONE_PADDING / 2)));
	// 	if (deadzone_prev_size == 0 || deadzone_prev_size > MAX_FIRST_POOL_SIZE) {
	// 		return nullptr;
	// 	}
	//
	// 	current_head = (pool_header_t *)((char *)current_head - deadzone_prev_size);
	// 	goto reloop;
}
#endif


pool_header_t *return_header(void *block_ptr)
{
	int search_attempts = 0;
	int offset = PD_HEAD_SIZE;
recheck:

	pool_header_t *header_candidate = (pool_header_t *)((u8 *)block_ptr - offset);

	// This should search 3 times for the max header offset value of ALIGNMENT.
	if (search_attempts > 2) {
		return nullptr;
	}

	const bool is_invalid_header = (header_candidate->chunk_size > 0 &&
	                                header_candidate->chunk_size < MAX_ALLOC_POOL_SIZE) == 0;

	if (is_invalid_header) {
		offset *= 2;
		search_attempts++;
		goto recheck;
	}
	return header_candidate;
}


inline int return_pool_array(memory_pool_t **arr)
{
	memory_pool_t *pool = arena_thread->first_mempool;

	int idx = 0;
	for (; idx < arena_thread->pool_count; idx++) {
		arr[idx] = pool;
		pool = pool->next_pool;
	}
	return idx;
}


void update_sentinel_and_free_flags(pool_header_t *head)
{
	const u32 prev_block_size = (head->bitflags & F_FIRST_HEAD)
	                            ? 0
	                            : (u32)(*((u8 *)head + head->chunk_size)) - (DEADZONE_PADDING / 2);

	constexpr u32 chunk_overflow = (KIBIBYTE * 128) + 1;

	// Branch inversion doesnt work for bitmaps (or maybe I just dont know how),
	// so it gets messy here. At least bitflags line up between pool_header_t and
	// pool_free_node_t, so no casting is needed for setting their bitflags.

	if (prev_block_size > 0 && prev_block_size < chunk_overflow) {
		pool_header_t *prev_head = (pool_header_t *)((u8 *)head - prev_block_size);
		if (prev_head->bitflags & F_SENTINEL) {
			prev_head->bitflags &= ~F_SENTINEL;
			head->bitflags |= F_SENTINEL;
		}
		if (prev_head->bitflags & F_FREE) {
			head->bitflags |= F_PREV_FREE;
		}
		if (head->bitflags & F_FREE) {
			prev_head->bitflags |= F_NEXT_FREE;
		}
		// TODO extract a lot of these branches into sub functions
		// TODO implement free list updating
	}
	if (head->bitflags & F_SENTINEL ||
	    (head->bitflags & F_FREE && ((pool_free_node_t *)head)->next_node != nullptr) ||
	    head->chunk_size > chunk_overflow) {
		return;
	}

	pool_header_t *next_head = (pool_header_t *)((u8 *)head + head->chunk_size);
	if (next_head->bitflags == 0 || next_head->bitflags & F_SENTINEL) {
		return;
	}

	if (next_head->bitflags & F_SENTINEL && head->bitflags & F_SENTINEL) {
		head->bitflags &= ~F_SENTINEL;
	}
	if (next_head->bitflags & F_FREE) {
		head->bitflags |= F_NEXT_FREE;
	}
	if (head->bitflags & F_FREE) {
		next_head->bitflags |= F_PREV_FREE;
	}
}


void pool_destructor()
{
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);
	if (!pool_arr_len) {
		return;
	}
	for (int i = pool_arr_len - 1; i >= 0; i--) {
		#ifdef ALLOC_DEBUG
		if (i != 0) {
			sync_alloc_log.to_console(log_stdout,
			                          "destroying pool of size: %lu at: %p\n",
			                          pool_arr[i]->size,
			                          pool_arr[i]->heap_base);
		} else {
			sync_alloc_log.to_console(log_stdout, "destroying core: %p\n", arena_thread);
		}
		#endif
		syn_unmap_page(pool_arr[i]->heap_base, pool_arr[i]->size);
	}
}