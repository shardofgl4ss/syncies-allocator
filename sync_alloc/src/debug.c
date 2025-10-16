//
// Created by SyncShard on 10/16/25.
//

#include "debug.h"

Debug_VTable logger ATTR_PRIVATE = {
	.debug_print_all = &debug_print_memory_usage,
	.to_console = &log_to_console
};

static void
debug_print_memory_usage(const Arena *restrict arena)
{
	const Memory_Pool *pool = arena->first_mempool;
	const Pool_Header *header = (Pool_Header *)arena->first_mempool->mem;
	const Handle_Table *handle_table = arena->first_hdl_tbl;

	u64 pool_count = 0, head_count = 0, free_headers = 0;
	u64 total_pool_mem = arena->total_mem_size;

	while (pool) {
		total_pool_mem += pool->pool_size;
		free_headers += pool->free_count;
		pool = pool->next_pool;
		pool_count++;
	}
	while (header) {
		header = header + header->size;
		head_count++;
	}

	printf("Total amount of pools: %lu\n", pool_count);
	printf("Pool memory (B): %lu\n", total_pool_mem);
	fflush(stdout);

	const usize header_bytes = head_count * PD_HEAD_SIZE;
	u64 handle_count = 0;
	u32 table_count = 0;

	do {
		const i32 curr_hdl_count = __builtin_popcountll(handle_table->entries_bitmap);
		if (curr_hdl_count >= 0)
			handle_count += (u64)curr_hdl_count;
		handle_table = handle_table->next_table;
		table_count++;
	}
	while (handle_table->next_table != nullptr);

	if (table_count != arena->table_count)
		fprintf(
			stderr,
			"SYNC_ALLOC: error: arena's table count: \"%u\" does not match actual table count: \"%u\"!\n",
			arena->table_count,
			table_count
		);

	const u64 handle_bytes = handle_count * PD_HANDLE_SIZE;
	const u64 reserved_mem = handle_bytes + header_bytes + (pool_count * PD_POOL_SIZE) + PD_ARENA_SIZE;

	printf("Struct memory of: headers: %lu\n", header_bytes);
	printf("Struct memory of: handles: %lu\n", handle_bytes);
	printf("Struct memory of: tables: %lu\n", (u64)table_count * PD_TABLE_SIZE);
	printf("Struct memory of: pools: %lu\n", pool_count * PD_POOL_SIZE);
	printf("Total reserved memory: %lu\n", reserved_mem);
	printf("Total memory used: %lu\n", total_pool_mem + reserved_mem);
	printf("Total count of free headers: %lu\n", free_headers);
	printf("Total count of handle tables: %u\n", table_count);

	fflush(stdout);
}