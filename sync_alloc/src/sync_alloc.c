//
// Created by SyncShard on 10/9/25.
//

#include "alloc_init.h"
#include "alloc_utils.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include "structs.h"
#include "sync_alloc.h"
#include <stdint.h>
#include <string.h>

//extern _Thread_local Arena *arena_thread;

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
	struct Arena_Handle user_handle = {
		.addr = nullptr,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = nullptr,
	};

	Memory_Pool *pool[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool);
	usize max_remaining_pool_space = 0;

	for (int i = 0; i < pool_arr_len; i++) {
		max_remaining_pool_space += (pool[i]->pool_size - pool[i]->pool_offset);
	}

	if ((usize)MAX_ALLOC_POOL_SIZE < size) {
		// return user handle for now as large page allocation isn't done yet.
		user_handle.generation = UINT32_MAX;
		return user_handle;
	}

	const u32 input_bytes = (u32)helper_add_padding(size);
	Pool_Header *new_head = find_new_pool_block(input_bytes);


memory_error:
	sync_alloc_log.to_console(log_stderr, "error, no memory!\n");
	user_handle.generation = UINT32_MAX;
	return user_handle;

arena_context_lost:
}

void syn_reset() {
	if (arena_thread == nullptr) {
		syn_panic("arena_thread was nullptr at: syn_reset()!\n");
	}
	Memory_Pool *pool = arena_thread->first_mempool;
	if (arena_thread->pool_count == 1 && pool != nullptr) {
		arena_thread->first_mempool->pool_offset = 0;
		return;
	}
	if (arena_thread->pool_count > 1 && pool != nullptr) {
		Memory_Pool *pool_arr[arena_thread->pool_count];
		const int ret = return_pool_array(pool_arr);
		pool_arr[0]->pool_offset = 0;
		for (int i = 1; i < ret; i++) {
			helper_destroy(pool_arr[i]->mem, pool_arr[i]->pool_size);
		}
		pool_arr[0]->next_pool = nullptr;
	}
}

void syn_free(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle) != 0) {
		return;
	}

	Pool_Header *head = user_handle->header;
	head->block_flags &= ~PH_ALLOCATED;
	if (head->block_flags & PH_SENSITIVE) {
		memset(head + PD_HEAD_SIZE, 0, head->size - PD_HEAD_SIZE);
		head->block_flags &= ~PH_SENSITIVE;
	}
	head->block_flags |= PH_FREE; // | PH_RECENT_FREE; // why do I have recent_free?
	user_handle->generation++;
	user_handle->addr = nullptr;

	update_sentinel_flags(head);
}

int syn_realloc(struct Arena_Handle *restrict user_handle, const usize size) {
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (bad_alloc_check(user_handle) != 0) {
		return 1;
	}
	#endif

	if ((size > UINT32_MAX) || user_handle == nullptr || user_handle->header->block_flags & PH_FROZEN) {
		return 1; // handle huge page reallocs later
	}

	// it's probably best to memcpy here

	Pool_Header *old_head = user_handle->header;
	Pool_Header *new_head = find_new_pool_block((u32)helper_add_padding(size));
	if (new_head == nullptr) {
		return 1;
	}
	user_handle->generation++;

	/* if there is a new head and nothing is null then it's all good to go for reallocation. */

	memcpy((char *)new_head + PD_HEAD_SIZE, (char *)old_head + PD_HEAD_SIZE, old_head->size - PD_HEAD_SIZE);
	user_handle->header = new_head;
	user_handle->addr = new_head + PD_HEAD_SIZE;
	new_head->block_flags = old_head->block_flags;

	if (old_head->block_flags & PH_SENSITIVE) {
		memset((char *)old_head + PD_HEAD_SIZE, 0, old_head->size - PD_HEAD_SIZE);
		old_head->block_flags &= ~PH_SENSITIVE;
	}
	old_head->block_flags &= ~PH_ALLOCATED;
	old_head->block_flags |= PH_FREE;

	update_table_generation(user_handle);
	return 0;
}

[[nodiscard]]
void *syn_freeze(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle)) {
		return nullptr;
	}

	user_handle->generation++;
	user_handle->header->block_flags |= PH_FROZEN;
	user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE);

	return user_handle->addr;
}

void syn_thaw(const struct Arena_Handle *restrict user_handle) {
	#ifndef SYN_ALLOC_DISABLE_SAFETY
	if (arena_thread == nullptr)
		goto arena_context_lost;
	if (corrupt_header_check(user_handle->header))
		goto corrupt_head;
	if (!(user_handle->header->block_flags & PH_FROZEN))
		goto nocrash_no_frozen;
	#endif

	update_table_generation(user_handle);
	if (!handle_generation_checksum(user_handle))
		goto nocrash_stale;
	user_handle->header->block_flags &= ~PH_FROZEN;
	// arena_defragment(arena, true);
	return;

	#ifndef SYN_ALLOC_DISABLE_SAFETY
nocrash_no_frozen:
	sync_alloc_log.to_console(log_stderr, "thaw attempt on already thawed handle!\n");
	return;
nocrash_stale:
	sync_alloc_log.to_console(log_stderr, "thaw attempt on stale handle!\n");
	return;
arena_context_lost:
	syn_panic("arena_thread was nullptr at syn_thaw()!\n");
corrupt_head:
	syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	#endif
}
