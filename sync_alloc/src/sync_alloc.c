//
// Created by SyncShard on 10/9/25.
//
// ReSharper disable CppUnusedIncludeDirective

#include "sync_alloc.h"
#include "alloc_init.h"
#include "alloc_utils.h"
#include "debug.h"
#include "defs.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include "structs.h"
#include "syn_memops.h"
#include "types.h"
#include <stdint.h>

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
	if (arena_init() != 0) {
		sync_alloc_log.to_console(log_stderr, "OOM\n");
		return invalid_hdl();
	}

arena_initialized:
	memory_pool_t *pool[arena_thread->pool_count + 1];
	int pool_arr_len = return_pool_array(pool);
	const u32 padded_size = (size < MINIMUM_BLOCK_ALLOC)
		? ADD_PADDING(MINIMUM_BLOCK_ALLOC)
		: ADD_PADDING(size);

	if ((usize)MAX_ALLOC_POOL_SIZE < size) {
		// TODO implement large page allocation
		return invalid_hdl();
	}
	bool retried = false;
reloop:
	pool_header_t *new_head = find_or_create_new_header(padded_size);
	if (retried) {
		return invalid_hdl();
	}
	if (new_head == nullptr) {
		pool[pool_arr_len] = pool_init(pool[pool_arr_len]->size * 2);
		pool_arr_len++;
		retried = true;
		goto reloop;
	}
	return create_handle_and_entry(new_head);
}


void syn_reset() {
	if (arena_thread == nullptr || arena_thread->pool_count == 0) {
		sync_alloc_log.to_console(log_stderr, "syn_reset() called without an allocated arena!\n");
		return;
	}
	if (arena_thread->pool_count == 1) {
		arena_thread->first_mempool->offset = 0;
		return;
	}
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);
	if (pool_arr_len == 0) {
		return;
	}
	pool_arr[0]->offset = 0;
	for (int i = 1; i < pool_arr_len; i++) {
		helper_destroy(pool_arr[i]->mem, pool_arr[i]->size);
	}
	pool_arr[0]->next_pool = nullptr;
}


void syn_free(struct Arena_Handle *restrict user_handle) {
	if (bad_alloc_check(user_handle, 1) != 0) {
		return;
	}

	pool_header_t *head = user_handle->header;
	head->bitflags &= ~PH_ALLOCATED;
	if (head->bitflags & PH_SENSITIVE) {
		syn_memset(head + PD_HEAD_SIZE, 0, head->size - PD_HEAD_SIZE);
		head->bitflags &= ~PH_SENSITIVE;
	}
	head->bitflags |= PH_FREE;
	user_handle->generation++;
	user_handle->addr = nullptr;


	update_sentinel_flags(head);
}


int syn_realloc(struct Arena_Handle *restrict user_handle, const usize size) {
	if (bad_alloc_check(user_handle, 1) != 0) {
		return 1;
	}

	if ((size > UINT32_MAX) || user_handle == nullptr || user_handle->header->bitflags & PH_FROZEN) {
		return 1; // handle huge page reallocs later
	}

	pool_header_t *old_head = user_handle->header;
	pool_header_t *new_head = find_or_create_new_header((u32)helper_add_padding(size));
	const void *old_block_data = ((char *)old_head + PD_HEAD_SIZE);
	void *new_block_data = ((char *)new_head + PD_HEAD_SIZE);
	const u32 new_block_data_size = old_head->size - PD_HEAD_SIZE;
	if (new_head == nullptr) {
		return 1;
	}
	user_handle->generation++;
	syn_memcpy(new_block_data, old_block_data, new_block_data_size);
	user_handle->header = new_head;
	user_handle->addr = new_head + PD_HEAD_SIZE;
	new_head->bitflags = old_head->bitflags;

	if (old_head->bitflags & PH_SENSITIVE) {
		syn_memset((char *)old_head + PD_HEAD_SIZE, 0, old_head->size - PD_HEAD_SIZE);
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


void syn_thaw(const syn_handle_t *restrict user_handle) {
	if (bad_alloc_check(user_handle, 1) != 2) {
		return;
	}

	update_table_generation(user_handle);
	if (!handle_generation_checksum(user_handle)) {
		sync_alloc_log.to_console(log_stderr, "thaw attempt on stale handle!\n");
		return;
	}
	user_handle->header->bitflags &= ~PH_FROZEN;
	// arena_defragment(arena, true);
}
