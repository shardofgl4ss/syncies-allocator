//
// Created by SyncShard on 10/9/25.
//
// ReSharper disable CppUnusedIncludeDirective

#include "sync_alloc.h"
#include "alloc_init.h"
#include "alloc_utils.h"
#include "debug.h"
#include "defs.h"
#include "free_node.h"
#include "internal_alloc.h"
#include "structs.h"
#include "syn_memops.h"
#include "types.h"

#ifndef SYN_USE_RAW
#include "handle.h"
#endif

#include <stdint.h>

void syn_destroy()
{
	if (arena_thread == nullptr || (arena_thread->pool_count == 0)) {
		return;
	}
	#ifndef SYN_USE_RAW
	if (arena_thread->table_count > 0) {
		table_destructor();
	}
	#endif
	if (arena_thread->pool_count > 0) {
		pool_destructor();
	}
	arena_thread = nullptr;
}


void syn_reset()
{
	if (arena_thread == nullptr) {
		sync_alloc_log.to_console(log_stderr, "syn_reset() called without an allocated arena!\n");
		return;
	}

	if (arena_thread->pool_count == 1) {
		arena_thread->first_mempool->offset = 0;
		arena_thread->first_mempool->free_count = 0;
		arena_thread->first_mempool->first_free = nullptr;
		table_destructor();
		return;
	}

	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);

	if (pool_arr_len == 0) {
		return;
	}
	pool_arr[0]->offset = 0;
	for (int i = pool_arr_len - 1; i > 0; i--) {
		syn_unmap_page(pool_arr[i]->mem, pool_arr[i]->size);
	}

	pool_arr[0]->offset = 0;
	pool_arr[0]->free_count = 0;
	pool_arr[0]->next_pool = nullptr;
	pool_arr[0]->first_free = nullptr;
	table_destructor();
}


[[nodiscard]]
#ifdef SYN_USE_RAW

void *syn_alloc(const usize size)
{

}

#else

syn_handle_t syn_alloc(const usize size)
{
	if (arena_thread != nullptr) {
		goto arena_initialized;
	}
	if (arena_init() != 0) {
		sync_alloc_log.to_console(log_stderr, "OOM\n");
		return invalid_block();
	}

arena_initialized:
	memory_pool_t *pool[arena_thread->pool_count + 1];
	int pool_arr_len = return_pool_array(pool);

	// TODO implement slabs for fast small-scale allocations
	const u32 padded_size = (size < MINIMUM_BLOCK_ALLOC)
	                        ? ADD_ALIGNMENT_PADDING(MINIMUM_BLOCK_ALLOC)
	                        : ADD_ALIGNMENT_PADDING(size);

	if ((usize)MAX_ALLOC_POOL_SIZE < size) {
		// TODO implement large page allocation
		return invalid_block();
	}
	bool retried = false;
reloop:
	pool_header_t *new_head = find_or_create_new_header(padded_size);
	if (retried) {
		return invalid_block();
	}
	if (new_head == nullptr) {
		pool[pool_arr_len] = pool_init(pool[pool_arr_len]->size * 2);
		pool_arr_len++;
		retried = true;
		goto reloop;
	}
	return create_handle_and_entry(new_head);
}
#endif


inline syn_handle_t syn_calloc(const usize size)
{
	const syn_handle_t hdl = syn_alloc(size);
	const bool is_invalid_hdl = (hdl.generation == UINT32_MAX ||
	                             hdl.header->handle_matrix_index == UINT32_MAX ||
	                             hdl.header == nullptr) != 0;

	if (is_invalid_hdl) {
		return invalid_block();
	}

	syn_memset(hdl.addr, 0, hdl.header->allocation_size);
	return hdl;
}


void syn_free(syn_handle_t *restrict user_handle)
{
	if (bad_alloc_check(user_handle, 1)) {
		return;
	}

	pool_header_t *head = user_handle->header;
	head->bitflags &= ~F_ALLOCATED;

	if (head->bitflags & F_SENSITIVE) {
		syn_memset((void *)BLOCK_ALIGN_PTR(head, ALIGNMENT), 0, head->allocation_size);
		head->bitflags &= ~F_SENSITIVE;
	}

	head->bitflags |= F_FREE;
	user_handle->generation++;
	user_handle->addr = nullptr;

	const u32 row = head->handle_matrix_index / MAX_TABLE_HNDL_COLS;
	const u32 col = head->handle_matrix_index % MAX_TABLE_HNDL_COLS;

	handle_table_t *table = arena_thread->first_hdl_tbl;

	for (u32 i = 1; i < row; i++) {
		table = table->next_table;
	}

	table->entries_bitmap &= ~(1ULL << col);
	memory_pool_t *pool = return_pool(head);

	// TODO move all free header handling to its own free_node.c file. Some handling is in alloc_utils.c/.h

	pool_free_node_t *node = (pool_free_node_t *)head;
	node->next_free = nullptr;
	free_node_add(pool, node);

	update_sentinel_and_free_flags(head);
}


int syn_realloc(syn_handle_t *restrict user_handle, const usize size)
{
	if (bad_alloc_check(user_handle, 1) != 0) {
		return 1;
	}

	if (size > (usize)MAX_ALLOC_POOL_SIZE) {
		return 1; // handle huge page reallocs later
	}

	pool_header_t *old_head = user_handle->header;
	pool_header_t *new_head = find_or_create_new_header(ADD_ALIGNMENT_PADDING(size));
	if (new_head == nullptr) {
		return 1;
	}

	const u32 new_block_data_size = old_head->chunk_size - PD_HEAD_SIZE;

	const void *old_block_data = ((char *)old_head + PD_HEAD_SIZE);
	void *new_block_data = ((char *)new_head + PD_HEAD_SIZE);

	user_handle->generation++;
	syn_memcpy(new_block_data, old_block_data, new_block_data_size);
	user_handle->header = new_head;
	user_handle->addr = (void *)BLOCK_ALIGN_PTR(new_head, ALIGNMENT);
	new_head->bitflags = old_head->bitflags;

	if (old_head->bitflags & F_SENSITIVE) {
		syn_memset((char *)old_head + PD_HEAD_SIZE, 0, old_head->chunk_size - PD_HEAD_SIZE);
		old_head->bitflags &= ~F_SENSITIVE;
	}

	old_head->bitflags &= ~F_ALLOCATED;
	old_head->bitflags |= F_FREE;

	update_table_generation(user_handle->header->handle_matrix_index);
	return 0;
}


void *syn_freeze(syn_handle_t *restrict user_handle)
{
	if (bad_alloc_check(user_handle, 1)) {
		return nullptr;
	}

	user_handle->header->bitflags |= F_FROZEN;
	user_handle->addr = (void *)BLOCK_ALIGN_PTR(user_handle->header, ALIGNMENT);

	update_table_generation(user_handle->header->handle_matrix_index);
	return user_handle->addr;
}


syn_handle_t syn_thaw(void *restrict block_ptr)
{
	if (!block_ptr) {
		return invalid_block();
	}

	pool_header_t *head = return_header(block_ptr);
	if (!head) {
		sync_alloc_log.to_console(log_stderr, "invalid block_ptr!\n");
		return invalid_block();
	}
	syn_handle_t *table_hdl = return_handle(head->handle_matrix_index);
	syn_handle_t user_hdl = *table_hdl;

	table_hdl->generation++;
	user_hdl.generation++;

	head->bitflags &= ~F_FROZEN;

	return user_hdl;
}
