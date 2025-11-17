//
// Created by SyncShard on 10/16/25.
//

#include "alloc_init.h"
#include "alloc_utils.h"
#include "deadzone.h"
#include "debug.h"
#include "defs.h"
#include "globals.h"
#include "handle.h"
#include "structs.h"
#include "types.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

_Thread_local arena_t *arena_thread = nullptr;


int syn_unmap_page(void *restrict mem, const usize bytes)
{
	return munmap(mem, bytes);
}


void *syn_map_page(const usize bytes)
{
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}


int arena_init()
{
	void *raw_pool = syn_map_page(MAX_FIRST_POOL_SIZE);

	if (raw_pool == MAP_FAILED) {
		goto alloc_failure;
	}

	arena_thread = raw_pool;

	memory_pool_t *first_pool = (memory_pool_t *)((char *)raw_pool + PD_ARENA_SIZE);
	const uintptr_t relative_cache_align = ALIGN_PTR(((uintptr_t)raw_pool
		                                                 + PD_RESERVED_F_SIZE
		                                                 + (DEADZONE_PADDING * 2)),
	                                                 64) - (uintptr_t)raw_pool;
	first_pool->heap_base = raw_pool;
	first_pool->mem = (void *)((char *)raw_pool + relative_cache_align);

	create_pool_deadzone(first_pool);
	const uintptr_t reserved_bytes = (uintptr_t)first_pool->mem - (uintptr_t)raw_pool;

	first_pool->offset = 0;
	first_pool->size = MAX_FIRST_POOL_SIZE - reserved_bytes;
	first_pool->next_pool = nullptr;

	arena_thread->total_arena_bytes = (usize)MAX_FIRST_POOL_SIZE;
	arena_thread->table_count = 0;
	arena_thread->first_hdl_tbl = new_handle_table();
	arena_thread->first_mempool = first_pool;
	arena_thread->pool_count = 1;

	return 0;

alloc_failure:
	#ifdef ALLOC_DEBUG
	sync_alloc_log.to_console(
	                          log_stderr,
	                          "OOM, when you buy more ram, send some to your local protogen too!\n");
	#endif
	return 1;
}


memory_pool_t *pool_init(const u32 size)
{
	const usize padded_size = ADD_ALIGNMENT_PADDING(size);
	void *raw_pool = syn_map_page(padded_size);

	if (!raw_pool) {
		return nullptr;
	}
	const uintptr_t relative_cache_align = ALIGN_PTR(
	                                                 ((uintptr_t)raw_pool + PD_POOL_SIZE + (DEADZONE_PADDING * 2)),
	                                                 64)
	                                       - (uintptr_t)raw_pool;
	memory_pool_t *new_pool = raw_pool;

	new_pool->heap_base = raw_pool;
	new_pool->mem = (void *)((char *)raw_pool + relative_cache_align);

	create_pool_deadzone(new_pool);
	const uintptr_t reserved_bytes = (uintptr_t)new_pool->mem - (uintptr_t)raw_pool;

	new_pool->size = padded_size - reserved_bytes;
	new_pool->free_count = 0;
	new_pool->offset = 0;
	new_pool->first_free = nullptr;
	new_pool->next_pool = nullptr;

	memory_pool_t *pool[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool);
	if (!pool_arr_len) {
		return nullptr;
	}
	memory_pool_t *prev_pool = pool[pool_arr_len - 1];
	prev_pool->next_pool = new_pool;

	arena_thread->total_arena_bytes += (new_pool->size + reserved_bytes);
	arena_thread->pool_count++;

	return new_pool;
}


_Noreturn void syn_panic(const char *panic_msg)
{
	const char *prefix = "SYNC ALLOC [PANIC] ";
	// Ill put debug printing here later with a global thread registry to be able to destroy all pools in
	// succession.
	fflush(stdout);
	write(2, prefix, strlen(prefix));
	write(2, panic_msg, strlen(panic_msg));
	fflush(stderr);

	raise(SIGABRT);
	// sigabrt unlikely to *not* terminate the application, but just in case.
	_exit(SIGABRT);
}