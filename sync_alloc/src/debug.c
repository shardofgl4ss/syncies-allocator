//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppParameterMayBeConst

#include "debug.h"
#include "globals.h"
#include "handle.h"
#include "structs.h"
#include "types.h"
#include <stdarg.h>
#include <stdbit.h>
#include <stdio.h>
#include <string.h>

extern _Thread_local arena_t *restrict arena_thread;


inline void log_stdout(const char *string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [LOG] ";
	fputs(prefix, stdout);
	vfprintf(stdout, string, arg_list);
	fflush(stdout);
}


inline void log_stderr(const char *restrict string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [ERR] ";
	fputs(prefix, stderr);
	vfprintf(stderr, string, arg_list);
	fflush(stderr);
}


void log_to_console(void (*log_and_stream)(const char *restrict format, va_list va_args),
                    const char *restrict str,
                    ...)
{
	/* I've heard it may be undefined to compare fptr's to functions, we'll see :3 */

	if (str == nullptr) {
		goto logger_start_failure;
	}
	if (strlen(str) == 0) {
		goto invalid_arg;
	}

	va_list arg_list = {};
	va_start(arg_list);

	log_and_stream(str, arg_list);

	va_end(arg_list);

	return;
invalid_arg:
	sync_alloc_log.to_console(log_stderr, "logger received invalid argument!\n");
logger_start_failure:
	sync_alloc_log.to_console(log_stderr, "logger failed to log!\n");
}


void debug_print_memory_usage()
{
	// TODO redo this entire function
	const memory_pool_t *pool = arena_thread->first_mempool;
	const pool_header_t *header = arena_thread->first_mempool->offset != 0
	                                      ? (pool_header_t *)arena_thread->first_mempool->mem
	                                      : nullptr;
	const handle_table_t *handle_table = arena_thread->first_hdl_tbl;

	u64 pool_count = 0;
	u64 free_headers = 0;
	u64 total_pool_mem = arena_thread->total_arena_bytes;

	while (pool != nullptr) {
		total_pool_mem += pool->size;
		free_headers += pool->free_count;
		pool = pool->next_pool;
		pool_count++;
	}

	sync_alloc_log.to_console(log_stdout, "Total amount of pools: %lu\n", pool_count);
	sync_alloc_log.to_console(log_stdout, "Pool memory (B): %lu\n", total_pool_mem);

	u64 handle_count = 0;
	u32 table_count = 0;

	if (handle_table != nullptr) {
		do {
			const u32 curr_hdl_count = stdc_count_ones(handle_table->entries_bitmap);
			if (curr_hdl_count != 0) {
				handle_count += (u64)curr_hdl_count;
			}
			handle_table = handle_table->next_table;
			table_count++;
		} while (handle_table->next_table != nullptr);
	}

	if (table_count != arena_thread->table_count) {
		sync_alloc_log.to_console(log_stderr,
		                          "arena_thread's table count: \"%u\" does not match "
		                          "actual table count: \"%u\"!\n",
		                          arena_thread->table_count,
		                          table_count);
	}

	const u64 handle_bytes = handle_count * STRUCT_SIZE_HANDLE;
	const u64 reserved_mem = handle_bytes + (pool_count * STRUCT_SIZE_POOL) + STRUCT_SIZE_ARENA;

	//sync_alloc_log.to_console(log_stdout, "Struct memory of: headers: %lu\n", header_bytes);
	sync_alloc_log.to_console(log_stdout, "Struct memory of: handles: %lu\n", handle_bytes);
	sync_alloc_log.to_console(log_stdout,
	                          "Struct memory of: tables: %lu\n",
	                          (u64)table_count * STRUCT_SIZE_HANDLE_TABLE);
	sync_alloc_log.to_console(log_stdout,
	                          "Struct memory of: pools: %lu\n",
	                          pool_count * STRUCT_SIZE_POOL);
	sync_alloc_log.to_console(log_stdout, "Total reserved memory: %lu\n", reserved_mem);
	sync_alloc_log.to_console(log_stdout,
	                          "Total memory used: %lu\n",
	                          total_pool_mem + reserved_mem);
	sync_alloc_log.to_console(log_stdout, "Total count of free headers: %lu\n", free_headers);
	sync_alloc_log.to_console(log_stdout, "Total count of handle tables: %u\n", table_count);
}

// clang-format off

debug_vtable_t sync_alloc_log = {
	.to_console = &log_to_console,
        .debug_print_all = &debug_print_memory_usage
};
