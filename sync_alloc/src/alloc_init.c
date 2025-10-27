//
// Created by SyncShard on 10/16/25.
//

#include "alloc_init.h"
#include "handle.h"
#include "helper_functions.h"
#include <signal.h>
#include <unistd.h>


_Thread_local Arena *restrict arena_thread = nullptr;

int
arena_init()
{
	void *raw_pool = helper_map_mem(
		(MAX_FIRST_POOL_SIZE + (ALIGNMENT - 1)) & (usize)~(ALIGNMENT - 1)
	);

	if (raw_pool == MAP_FAILED)
		goto alloc_failure;

	arena_thread = raw_pool;

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set pool_size to the FIRST_POOL_ALLOC.					*
	 *	This spoofs a max memory of POOL_SIZE when its actually POOL_SIZE - reserved.				*
	 *  Alignment is added to the pool in case the user changes it to 16 or 32, it will be aligned.	*/

	auto *first_pool = (Memory_Pool *)((char *)raw_pool + PD_ARENA_SIZE);

	first_pool->mem = (void *)((char *)raw_pool + PD_RESERVED_F_SIZE + DEADZONE_PADDING);
	auto *deadzone = (u64 *)((char *)first_pool->mem - DEADZONE_PADDING);
	*deadzone = HEAP_DEADZONE;

	first_pool->pool_offset = 0;
	first_pool->pool_size = MAX_FIRST_POOL_SIZE;
	first_pool->next_pool = nullptr;
	first_pool->prev_pool = nullptr;

	arena_thread->total_mem_size = MAX_FIRST_POOL_SIZE;
	arena_thread->table_count = 0;
	arena_thread->first_hdl_tbl = mempool_new_handle_table(arena_thread->first_hdl_tbl);
	arena_thread->first_mempool = first_pool;
	arena_thread->pool_count = 1;

	return 0;

alloc_failure:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(
		log_stderr,
		"arena allocation failure! when you buy more ram, send some to your local protogen too!\n"
	);
	#endif
	return 1;
}

Memory_Pool *
pool_init(const u32 size)
{
	if (arena_thread == nullptr)
		syn_panic("arena_thread was nullptr at: pool_init()!\n");
	Memory_Pool *pool = arena_thread->first_mempool;

	if (pool == nullptr) goto mp_internal_error;
	while (pool->next_pool != nullptr) pool = pool->next_pool;

	void *raw = helper_map_mem(size + PD_POOL_SIZE);
	if (raw == nullptr) goto mp_internal_error;

	Memory_Pool *new_pool = raw;

	new_pool->mem = (void *)((char *)raw + PD_POOL_SIZE);
	pool->next_pool = new_pool;
	new_pool->prev_pool = pool;
	new_pool->pool_size = size;
	new_pool->pool_offset = 0;
	new_pool->free_count = 0;
	new_pool->next_pool = nullptr;
	new_pool->first_free = nullptr;

	arena_thread->total_mem_size += new_pool->pool_size;
	arena_thread->pool_count++;

	return new_pool;

mp_internal_error:
	#if defined(ALLOC_DEBUG)
	sync_alloc_log.to_console(log_stderr, "failed to create internal memory pool!\n");
	#endif
	return nullptr;
}

_Noreturn void
syn_panic(const char *panic_msg)
{
	const char *prefix = "SYNC ALLOC [PANIC] ";
	// Ill put debug printing here later with a global thread registry to be able to destroy all pools in succession.
	fflush(stdout);
	write(2, prefix, strlen(prefix));
	write(2, panic_msg, strlen(panic_msg));
	fflush(stderr);

	raise(SIGABRT);
	// sigabrt unlikely to *not* terminate the application, but just in case.
	_exit(SIGABRT);
}
