//
// Created by SyncShard on 10/9/25.
//

#include "sync_alloc.h"
#include "alloc_init.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include <stdint.h>
#include <string.h>

/*
 * It's a pain in the ass to work with handles, I might completely drop them for just raw voidptrs.
 */

extern _Thread_local Arena *arena_thread;

int
arena_create()
{
	arena_init();
	if (arena_thread == nullptr)
		goto arena_thread_fail;

	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stdout, "Arena creation successful! addr: %p\n");
	#endif

	return 0;

arena_thread_fail:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(
		log_stdout,
		"arena allocation failure! When you buy more ram, send some to your local protogen too!\n"
	);
	#endif
	// We dont panic here because other threads may have been successful and still have valid memory.
	// I might write a global thread registry later to keep track of them all,
	// and make panic unwinding for the heap possible.
	return 1;
}


void
arena_destroy()
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr)
		return;
	const Memory_Pool *pool = arena_thread->first_mempool;

	if (pool->next_pool == nullptr) {
		#if defined(ALLOC_DEBUG)
		sync_alloc_log.to_console(log_stdout, "destroying pool: %p\n", pool->mem);
		#endif
		mp_helper_destroy(pool->mem, pool->pool_size + PD_RESERVED_F_SIZE);
		return;
	}

	pool = pool->next_pool;
	mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_RESERVED_F_SIZE);
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stdout, "destroying pool: %p\n", pool->mem);
	#endif

	while (pool->next_pool != nullptr) {
		pool = pool->next_pool;
		#if defined(ALLOC_DEBUG)
		sync_alloc_log.to_console(log_stdout, "destroying pool: %p\n", pool->mem);
		#endif
		mp_helper_destroy(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
	}

	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stdout, "destroying arena_thread: %p!\n", arena_thread);
	#endif
	mp_helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);
}


// figure out defragment later
//void
//arena_defragment(const Arena *const restrict arena, const bool l_defrag)
//{
//	Memory_Pool *restrict pool = (arena) ? arena->first_mempool : nullptr;
//	Pool_Header *restrict head = (pool->pool_offset != 0) ? pool->mem : nullptr;
//
//	if (pool == nullptr
//	    || head == nullptr
//	    || pool->pool_size == 0
//	    || (
//		    pool->pool_offset == 0
//		    && pool->next_pool == nullptr
//	    )
//	) { return; }
//}


[[nodiscard]] struct Arena_Handle
arena_alloc(const usize size)
{
	if (arena_thread == nullptr) {
		sync_alloc_log.to_console(
			log_stderr,
			"PANICKING: arena_thread TLS is nullptr at function: arena_alloc()!\n"
		);
		arena_panic();
	}

	struct Arena_Handle user_handle = {
		.addr = nullptr,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = nullptr,
	};

	Memory_Pool *pool = arena_thread->first_mempool;

	if (MAX_ALLOC_POOL_SIZE < size) // return user handle for now as large page allocation isn't done yet.
		return user_handle;

	const u32 input_bytes = (u32)mp_helper_add_padding(size);

	#if defined(ALLOC_DEBUG)
	if (input_bytes != size) {
		sync_alloc_log.to_console(
			log_stdout,
			"debug: padding has been added. input: %lu, padded: %lu\n",
			size,
			input_bytes);
	}
	#endif

	Pool_Header *head;
	do {
		head = mempool_find_block(input_bytes);
		if (head == nullptr) {
			if (pool->next_pool != nullptr) {
				while (pool->next_pool != nullptr)
					pool = pool->next_pool;
			}

			Memory_Pool *new_pool = pool_init(pool->pool_size * 2);
			if (new_pool == nullptr)
				goto memory_error;
			pool->next_pool = new_pool;
		}
	}
	while (head == nullptr);

	user_handle = mempool_create_handle_and_entry(head);
	return user_handle;

memory_error:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "out of memory, but trying to continue!\n");
	#endif
	user_handle.generation = UINT32_MAX;
	return user_handle;
}


void
arena_reset(const int reset_type)
{
	Memory_Pool *restrict pool = arena_thread->first_mempool;

	switch (reset_type) {
		default:
		case 1:
			pool->pool_offset = 0;
			return;
		case 0:
			pool->pool_offset = 0;
			if (!pool->next_pool)
				return;

			while (pool->next_pool) {
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = nullptr;
				pool->prev_pool = nullptr;
			};
			break;
		case 2:
			if (!pool->next_pool)
				return;

			while (pool->next_pool) {
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = nullptr;
				pool->prev_pool = nullptr;
			};
			break;
	}
}


void update_sentinel_flags(Pool_Header *head)
{
	if (!(head->block_flags & PH_SENTINEL_F)) {
		Pool_Header *prev_head = head - head->size;
		if (prev_head != nullptr && prev_head->block_flags & PH_FREE) {
			head->block_flags |= PH_PREV_FREE;
			prev_head->block_flags |= PH_NEXT_FREE;
		}
	}
	if (!(head->block_flags & PH_SENTINEL_L)) {
		Pool_Header *next_head = head + head->size;
		if (next_head != nullptr && next_head->block_flags & PH_FREE) {
			head->block_flags |= PH_NEXT_FREE;
			next_head->block_flags |= PH_PREV_FREE;
		}
	}
}


void
arena_free(struct Arena_Handle *user_handle)
{
	if (!mp_helper_handle_generation_checksum(user_handle)) {
		sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
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

	//arena_defragment(0);
}


int
arena_realloc(struct Arena_Handle *restrict user_handle, const usize size)
{
	if ((size > UINT32_MAX)
		|| user_handle == nullptr
		|| user_handle->header->block_flags & PH_FROZEN)
		return 1; // handle huge page reallocs later

	// it's probably best to memcpy here

	Pool_Header *old_head = user_handle->header;
	Pool_Header *new_head = mempool_find_block((u32)mp_helper_add_padding(size));
	if (new_head == nullptr) return 1;
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

	mp_helper_update_table_generation(user_handle);
	return 0;
}


[[nodiscard]] void *
handle_lock(struct Arena_Handle *restrict user_handle)
{
	if (!mp_helper_handle_generation_checksum(user_handle))
		goto handle_stale;
	if (user_handle->header->block_flags & PH_FROZEN)
		goto handle_stale_lock;

	user_handle->generation++;
	user_handle->header->block_flags |= PH_FROZEN;
	user_handle->addr = (void *)((char*)user_handle->header + PD_HEAD_SIZE);

	return user_handle->addr;

handle_stale_lock:
	sync_alloc_log.to_console(log_stderr, "handle already locked!\n");
handle_stale:
	sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
	return nullptr;
}


void
handle_unlock(const struct Arena_Handle *user_handle)
{
	mp_helper_update_table_generation(user_handle);

	if (!mp_helper_handle_generation_checksum(user_handle)) {
		sync_alloc_log.to_console(log_stderr, "stale handle detected!\n");
		return;
	}
	user_handle->header->block_flags &= ~PH_FROZEN;

	//arena_defragment(arena, true);
}
