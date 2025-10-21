//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppRedundantBooleanExpressionArgument
// ReSharper disable CppDFAUnreachableCode

#include "debug.h"
#include "unistd.h"
#include <stdarg.h>
#include <stdbit.h>
#include <stdio.h>
#include <string.h>

void
log_to_console(void (*log_and_stream)(const char *restrict format, va_list va_args), const char *restrict str, ...)
{
	/* I've heard it may be undefined to compare fptr's to functions, oh well */
	if (!ALLOC_DEBUG_LVL || (ALLOC_DEBUG_LVL == 1 && log_and_stream != log_stderr))
		return;

	if (str == nullptr)
		goto logger_start_failure;
	if (strlen(str) == 0)
		goto invalid_arg;

	va_list arg_list;
	va_start(arg_list);

	log_and_stream(str, arg_list);

	va_end(arg_list);

	return;
invalid_arg:
	perror("SYNC_ALLOC [ERR] logger received invalid argument!\n");
logger_start_failure:
	perror("SYNC_ALLOC [ERR] logger failed to log!\n");
	fflush_unlocked(stdout);
}

void
debug_print_memory_usage(const Arena *restrict arena)
{
	const Memory_Pool *pool = arena->first_mempool;
	const Pool_Header *header = arena->first_mempool->pool_offset != 0
	                            ? (Pool_Header *)arena->first_mempool->mem
	                            : nullptr;
	const Handle_Table *handle_table = arena->first_hdl_tbl;

	u64 pool_count = 0, head_count = 0, free_headers = 0;
	u64 total_pool_mem = arena->total_mem_size;

	while (pool != nullptr) {
		total_pool_mem += pool->pool_size;
		free_headers += pool->free_count;
		pool = pool->next_pool;
		pool_count++;
	}
	while (header != nullptr) {
		header = header + header->size;
		head_count++;
	}

	sync_alloc_log.to_console(log_stdout, "Total amount of pools: %lu\n", pool_count);
	sync_alloc_log.to_console(log_stdout, "Pool memory (B): %lu\n", total_pool_mem);
	fflush_unlocked(stdout);

	const usize header_bytes = head_count != 0 ? head_count * PD_HEAD_SIZE : 0;
	u64 handle_count = 0;
	u32 table_count = 0;

	if (handle_table != nullptr) {
		do {
			const u32 curr_hdl_count = stdc_count_ones(handle_table->entries_bitmap);
			if (curr_hdl_count != 0)
				handle_count += (u64)curr_hdl_count;
			handle_table = handle_table->next_table;
			table_count++;
		}
		while (handle_table->next_table != nullptr);
	}

	if (table_count != arena->table_count)
		sync_alloc_log.to_console(
			log_stderr,
			"SYNC_ALLOC: error: arena's table count: \"%u\" does not match actual table count: \"%u\"!\n",
			arena->table_count,
			table_count
		);

	const u64 handle_bytes = handle_count * PD_HANDLE_SIZE;
	const u64 reserved_mem = handle_bytes + header_bytes + (pool_count * PD_POOL_SIZE) + PD_ARENA_SIZE;

	sync_alloc_log.to_console(log_stdout, "Struct memory of: headers: %lu\n", header_bytes);
	sync_alloc_log.to_console(log_stdout, "Struct memory of: handles: %lu\n", handle_bytes);
	sync_alloc_log.to_console(log_stdout, "Struct memory of: tables: %lu\n", (u64)table_count * PD_TABLE_SIZE);
	sync_alloc_log.to_console(log_stdout, "Struct memory of: pools: %lu\n", pool_count * PD_POOL_SIZE);
	sync_alloc_log.to_console(log_stdout, "Total reserved memory: %lu\n", reserved_mem);
	sync_alloc_log.to_console(log_stdout, "Total memory used: %lu\n", total_pool_mem + reserved_mem);
	sync_alloc_log.to_console(log_stdout, "Total count of free headers: %lu\n", free_headers);
	sync_alloc_log.to_console(log_stdout, "Total count of handle tables: %u\n", table_count);

	fflush_unlocked(stdout);
}

Debug_VTable sync_alloc_log = {
	.to_console = &log_to_console,
	.debug_print_all = &debug_print_memory_usage
};
