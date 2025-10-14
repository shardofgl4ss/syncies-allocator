//
// Created by SyncShard on 10/9/25.
//

#include "include/internal_alloc.h"
#include "alloc_lib.h"

static Arena_Handle
mempool_create_handle_and_entry(Arena *restrict arena, Pool_Header *restrict head)
{
	Arena_Handle hdl = {};
	Handle_Table *table = arena->first_hdl_tbl;
	if (head == nullptr || arena == nullptr || arena->first_mempool == nullptr) { return hdl; }

reloop_hdl_tbl:
	while (MAX_TABLE_HNDL_COLS == __builtin_popcountll(table->entries_bitmap))
	{
		if (table->next_table != nullptr)
			table = table->next_table;
		else
		{
			table->next_table = mempool_new_handle_table(arena, table);
			if (table->next_table != nullptr)
				return hdl;
			table = table->next_table;
		}
	}

	const i32 handle_idx = __builtin_ctzll(~table->entries_bitmap);
	if (handle_idx == -1 || handle_idx < 0)
		goto reloop_hdl_tbl;
	table->entries_bitmap |= (1ull << handle_idx);

	hdl.header = head;
	hdl.addr = (void *)((char *)head + PD_HEAD_SIZE);
	hdl.handle_matrix_index = arena->table_count * MAX_TABLE_HNDL_COLS + (u32)handle_idx;
	hdl.generation = 1;

	table->handle_entries[handle_idx] = hdl;
	head->handle_idx = hdl.handle_matrix_index;
	return hdl;
}


static Handle_Table *
mempool_new_handle_table(Arena *restrict arena, Handle_Table *restrict table)
{
	if (table == nullptr)
		goto fail_null;

	const u32 new_id = ++arena->table_count;
	Handle_Table *new_tbl = mempool_map_mem(PD_HDL_MATRIX_SIZE);

	if (new_tbl == nullptr)
		goto fail_alloc;

	table->next_table = new_tbl;

	new_tbl->entries_bitmap = 0;
	new_tbl->table_id = new_id;

	return new_tbl;
fail_alloc:
	fprintf(stderr, "ERR_NO_MEMORY: failed to allocate table: %hu\n", new_id);
	arena->table_count--;

fail_null:
	return nullptr;
}


Pool_Header *
mempool_create_header(const Memory_Pool *restrict pool, const u16 size, const usize offset)
{
	if ((pool->pool_size - pool->pool_offset) < (mempool_add_padding(size) + PD_HEAD_SIZE))
		goto mp_head_create_error;

	auto *head = (Pool_Header *)((char *)pool->mem + offset);

	head->size = (u16)((size + PD_HEAD_SIZE) << 3);
	head->size = (head->size & ~0x7) | ((u8)ALLOCATED & 0x7);
	head->handle_idx = 0;
	head->prev_block_size = 0;

	return head;

mp_head_create_error:
	printf("error: not enough room in pool for header!\n");
	fflush(stdout);
	return nullptr;
}


static Memory_Pool *
mempool_create_internal_pool(Arena *restrict arena, const size_t size)
{
	if (arena == nullptr)
		goto mp_internal_error;

	Memory_Pool *pool = arena->first_mempool ? arena->first_mempool : nullptr;

	if (pool == nullptr)
		goto mp_internal_error;
	if (pool->next_pool)
		while (pool->next_pool) pool = pool->next_pool;

	void *raw = mempool_map_mem(size + PD_POOL_SIZE);

	if (raw == nullptr)
		goto mp_internal_error;

	Memory_Pool *new_pool = raw;
	new_pool->mem = (void *)((char *)raw + PD_POOL_SIZE);

	pool->next_pool = new_pool;
	new_pool->prev_pool = pool;

	new_pool->pool_size = size;
	new_pool->pool_offset = 0;
	new_pool->next_pool = nullptr;

	arena->total_mem_size += new_pool->pool_size;

	return new_pool;

mp_internal_error:
	perror("error: failed to create internal memory pool!\n");
	fflush(stdout);
	return nullptr;
}


static Pool_Header *
mempool_find_block(Arena *arena, const size_t requested_size)
{
	if (!arena || requested_size == 0 || arena->first_mempool->pool_size == 0) { return nullptr; }
	Memory_Pool *pool = arena->first_mempool;
	u_int32_t pool_idx = 0;

	// there is no first header if the offset is zero.
	if (pool->pool_offset == 0) { return mempool_create_header(pool, requested_size, pool->pool_offset, pool_idx); }

	const size_t new_chunk_size = requested_size + PD_HEAD_SIZE;
	auto *head = (Pool_Header *)((char *)pool->mem);

	while (head->next_header != nullptr)
	{
		if (head->flags == FREE)
		{
			if (head->block_size == requested_size || head->block_size < new_chunk_size) { return head; }
			if (head->block_size >= new_chunk_size)
			{
				// we cant use the create header function because we cant
				// figure out the offset from this specific header.
				const size_t tombstoned_blocks = head->block_size - new_chunk_size;
				auto *new_head = (Pool_Header *)((char *)head + tombstoned_blocks);
				head->block_size -= tombstoned_blocks;
				new_head->block_size = requested_size;
				new_head->flags = ALLOCATED;

				if (head->next_header != nullptr) { new_head->next_header = head->next_header; }

				new_head->prev_header = head;
				head->next_header = new_head;

				arena_defragment(arena, true);

				return new_head;
			}
		}
	}
}