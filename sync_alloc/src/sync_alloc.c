//
// Created by SyncShard on 10/9/25.
//

#include "sync_alloc.h"
#include "alloc_init.h"
#include "alloc_utils.h"
#include "handle.h"
#include "helper_functions.h"
#include "internal_alloc.h"
#include <stdint.h>
#include <string.h>

extern _Thread_local Arena *arena_thread;


void
syn_free_all()
{
	if (arena_thread == nullptr || (arena_thread->pool_count == 0 && arena_thread->table_count == 0))
		return;
	if (arena_thread->table_count > 0)
		table_destructor();
	if (arena_thread->pool_count > 0)
		pool_destructor();
	arena_thread = nullptr;
}


[[nodiscard]] struct Arena_Handle
syn_alloc(const usize size)
{
	if (arena_thread == nullptr) {
		if (arena_init() != 0)
			goto arena_context_lost;
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

	const u32 input_bytes = (u32)helper_add_padding(size);

	#if defined(ALLOC_DEBUG)
	if (input_bytes != size) {
		sync_alloc_log.to_console(
			log_stdout,
			"alignment padding has been added. input: %lu, padded: %lu\n",
			size,
			input_bytes);
	}
	#endif

	Pool_Header *head;
	do {
		if (corrupt_pool_check(pool)) goto alloc_corrupt;
		head = mempool_find_block(input_bytes);
		if (head == nullptr) {
			if (pool->next_pool != nullptr) {
				pool = pool->next_pool;
				continue;
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

arena_context_lost:
	syn_panic("arena_thread was nullptr at: arena_alloc()!\n");
alloc_corrupt:
	syn_panic("allocator structure <Arena> or <Memory_Pool> corruption detected!\n");
}


void
syn_reset()
{
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (arena_thread == nullptr) goto arena_context_lost;
	#endif

	Memory_Pool *restrict pool = arena_thread->first_mempool;
	pool->pool_offset = 0;
	if (arena_thread->pool_count == 1) return;

	pool = pool->next_pool;
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (corrupt_pool_check(pool)) goto alloc_corrupt;
	#endif

	for (u32 i = 2; i < arena_thread->pool_count; i++) {
		if (pool->next_pool == nullptr) {
			helper_destroy(pool->mem, pool->pool_size + PD_POOL_SIZE);
			return;
		}
		pool = pool->next_pool;

		#if !defined(SYN_ALLOC_DISABLE_SAFETY)
		if (corrupt_pool_check(pool)) goto alloc_corrupt;
		#endif

		munmap(pool->prev_pool->mem, pool->prev_pool->pool_size + PD_POOL_SIZE);
		pool->prev_pool->next_pool = nullptr;
		pool->prev_pool = nullptr;
	}

	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
arena_context_lost:
	syn_panic("arena_thread was nullptr at: syn_reset()!\n");
alloc_corrupt:
	syn_panic("allocator structure <Arena> or <Memory_Pool> corruption detected!\n");
	#endif
}


void
syn_free(struct Arena_Handle *restrict user_handle)
{
	// the cost of safety: everything has ifs :(
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (arena_thread == nullptr) goto arena_context_lost;
	if (corrupt_header_check(user_handle->header)) goto corrupt_head;
	if (!handle_generation_checksum(user_handle)) goto nocrash_stale;
	if (user_handle->header->block_flags & PH_FROZEN) goto nocrash_frozen;
	#endif

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
	return;

	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
nocrash_frozen:
	sync_alloc_log.to_console(log_stderr, "free attempt on allocation in use!\n");
	return;
nocrash_stale:
	sync_alloc_log.to_console(log_stderr, "double free detected!\n");
	return;
arena_context_lost:
	syn_panic("arena_thread was nullptr at syn_free()!\n");
corrupt_head:
	syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	#endif
}


int
syn_realloc(struct Arena_Handle *restrict user_handle, const usize size)
{
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (arena_thread == nullptr) goto arena_context_lost;
	if (corrupt_header_check(user_handle->header)) goto corrupt_head;
	if (!handle_generation_checksum(user_handle)) goto nocrash_stale;
	if (user_handle->header->block_flags & PH_FROZEN) goto nocrash_frozen;
	#endif

	if ((size > UINT32_MAX)
	    || user_handle == nullptr
	    || user_handle->header->block_flags & PH_FROZEN)
		return 1; // handle huge page reallocs later

	// it's probably best to memcpy here

	Pool_Header *old_head = user_handle->header;
	Pool_Header *new_head = mempool_find_block((u32)helper_add_padding(size));
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

	update_table_generation(user_handle);
	return 0;

	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
nocrash_frozen:
	sync_alloc_log.to_console(log_stderr, "realloc attempt on allocation in use!\n");
	return 1;
nocrash_stale:
	sync_alloc_log.to_console(log_stderr, "realloc on freed allocation!\n");
	return 1;
arena_context_lost:
	syn_panic("arena_thread was nullptr at syn_realloc()!\n");
corrupt_head:
	syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	#endif
}


[[nodiscard]] void *
syn_freeze(struct Arena_Handle *restrict user_handle)
{
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (arena_thread == nullptr) goto arena_context_lost;
	if (corrupt_header_check(user_handle->header)) goto corrupt_head;
	if (!handle_generation_checksum(user_handle)) goto nocrash_stale;
	if (user_handle->header->block_flags & PH_FROZEN) goto nocrash_frozen;
	#endif

	user_handle->generation++;
	user_handle->header->block_flags |= PH_FROZEN;
	user_handle->addr = (void *)((char *)user_handle->header + PD_HEAD_SIZE);

	return user_handle->addr;

	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
nocrash_frozen:
	sync_alloc_log.to_console(log_stderr, "realloc attempt on allocation in use!\n");
	return nullptr;
nocrash_stale:
	sync_alloc_log.to_console(log_stderr, "realloc on freed allocation!\n");
	return nullptr;
arena_context_lost:
	syn_panic("arena_thread was nullptr at syn_freeze()!\n");
corrupt_head:
	syn_panic("allocator structure <Pool_Header> corruption detected!\n");
	#endif
}


void
syn_thaw(const struct Arena_Handle *user_handle)
{
	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
	if (arena_thread == nullptr) goto arena_context_lost;
	if (corrupt_header_check(user_handle->header)) goto corrupt_head;
	if ((user_handle->header->block_flags & PH_FROZEN)) goto nocrash_no_frozen;
	#endif

	update_table_generation(user_handle);
	if (!handle_generation_checksum(user_handle)) goto nocrash_stale;
	user_handle->header->block_flags &= ~PH_FROZEN;
	//arena_defragment(arena, true);

	#if !defined(SYN_ALLOC_DISABLE_SAFETY)
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
