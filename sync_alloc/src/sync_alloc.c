//
// Created by SyncShard on 10/9/25.
//

#include "alloc_init.h"
#include "alloc_utils.h"
#include "debug.h"
#include "defs.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include "structs.h"
#include "sync_alloc.h"
#include "types.h"
#include <stdint.h>
#include <string.h>

// extern _Thread_local Arena *arena_thread;

void syn_free_all() {
	if (arena_thread == nullptr || (arena_thread->pool_count == 0 && arena_thread->table_count == 0)) {
		return;
	}
	if (arena_thread->table_count > 0) {
		table_destructor();
	}
	if (arena_thread->pool_count > 0) {
		pool_destructor();
	}
	arena_thread = nullptr;
}

[[nodiscard]]
struct Arena_Handle syn_alloc(const usize size) {
	if (arena_thread != nullptr) {
		goto arena_initialized;
	}
	arena_init();
	/* if its still null after initializing, then there is 			*
	 * something wrong with the arena_init function or the system is OOM,	*
	 * since TLS storage should never just spontaneously become invalid.	*/
	if (arena_thread == nullptr) {
		syn_panic("arena_thread was nullptr at: arena_alloc()!\n");
	}

arena_initialized:
	syn_handle_t user_handle = {};
	memory_pool_t *pool[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool);

	// I might remove the extra addition of ALIGNMENT, but at the moment,
	// its for adding extra padding to the user's allocation.
	// This means at least a small overwrite past buffers
	// wont immediately nuke the allocator, or cause problems.

	const u32 padded_size = (size > ALIGNMENT)
		? helper_add_padding(size) + ALIGNMENT
		: helper_add_padding(ALIGNMENT) + ALIGNMENT;

	if ((usize)MAX_ALLOC_POOL_SIZE < size) {
		// return user handle for now as large page allocation isn't done yet.
		user_handle.generation = UINT32_MAX;
		return user_handle;
	}
	bool cycle = false;
retry:
	pool_header_t *new_head = find_or_create_new_pool_block(padded_size);
	if (new_head != nullptr) {
		user_handle = mempool_create_handle_and_entry(new_head);
		return user_handle;
	}
	if (cycle) {
		sync_alloc_log.to_console(log_stderr, "OOM\n");
		return user_handle;
	}
	pool_init(pool[pool_arr_len]->size * 2);
	cycle = true;
	goto retry;
}

void syn_reset() {
	if (arena_thread == nullptr) {
		syn_panic("arena_thread was nullptr at: syn_reset()!\n");
	}
	memory_pool_t *pool = arena_thread->first_mempool;
	if (arena_thread->pool_count == 1 && pool != nullptr) {
		arena_thread->first_mempool->offset = 0;
		return;
	}
	if (arena_thread->pool_count > 1 && pool != nullptr) {
		memory_pool_t *pool_arr[arena_thread->pool_count];
		const int ret = return_pool_array(pool_arr);
		pool_arr[0]->offset = 0;
		for (int i = 1; i < ret; i++) {
			helper_destroy(pool_arr[i]->mem, pool_arr[i]->size);
		}
		pool_arr[0]->next_pool = nullptr;
	}
}

void syn_free(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle, 1) != 0) {
		return;
	}

	pool_header_t *head = user_handle->header;
	head->bitflags &= ~PH_ALLOCATED;
	if (head->bitflags & PH_SENSITIVE) {
		memset(head + PD_HEAD_SIZE, 0, head->size - PD_HEAD_SIZE);
		head->bitflags &= ~PH_SENSITIVE;
	}
	head->bitflags |= PH_FREE; // | PH_RECENT_FREE; // why do I have recent_free?
	user_handle->generation++;
	user_handle->addr = nullptr;

	update_sentinel_flags(head);
}

int syn_realloc(struct Arena_Handle *restrict user_handle, const usize size) {
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (bad_alloc_check(user_handle, 1) != 0) {
		return 1;
	}
	#endif

	if ((size > UINT32_MAX) || user_handle == nullptr || user_handle->header->bitflags & PH_FROZEN) {
		return 1; // handle huge page reallocs later
	}

	// it's probably best to memcpy here

	pool_header_t *old_head = user_handle->header;
	pool_header_t *new_head = find_or_create_new_pool_block((u32)helper_add_padding(size));
	if (new_head == nullptr) {
		return 1;
	}
	user_handle->generation++;

	/* if there is a new head and nothing is null then it's all good to go for reallocation. */

	memcpy((char *)new_head + PD_HEAD_SIZE, (char *)old_head + PD_HEAD_SIZE, old_head->size - PD_HEAD_SIZE);
	user_handle->header = new_head;
	user_handle->addr = new_head + PD_HEAD_SIZE;
	new_head->bitflags = old_head->bitflags;

	if (old_head->bitflags & PH_SENSITIVE) {
		memset((char *)old_head + PD_HEAD_SIZE, 0, old_head->size - PD_HEAD_SIZE);
		old_head->bitflags &= ~PH_SENSITIVE;
	}
	old_head->bitflags &= ~PH_ALLOCATED;
	old_head->bitflags |= PH_FREE;

	update_table_generation(user_handle);
	return 0;
}

void *syn_freeze(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle, 1)) {
		return nullptr;
	}

	user_handle->generation++;
	user_handle->header->bitflags |= PH_FROZEN;
	user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE);

	return user_handle->addr;
}

void syn_thaw(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle, 1) != 2) {
		return;
	}

	update_table_generation(user_handle);
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (!handle_generation_checksum(user_handle)) {
		goto nocrash_stale;
	}
	#endif
	user_handle->header->bitflags &= ~PH_FROZEN;
	// arena_defragment(arena, true);
	return;

	#ifndef SYN_ALLOC_DISABLE_SAFETY
nocrash_stale:
	sync_alloc_log.to_console(log_stderr, "thaw attempt on stale handle!\n");
	#endif
}
