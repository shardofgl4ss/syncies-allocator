//
// Created by SyncShard on 10/9/25.
//

#include "../include/internal_alloc.h"


static Arena_Handle
mempool_create_handle_and_entry(Arena *restrict arena, Mempool_Header *restrict head)
{
	Arena_Handle hdl = {};
	if (head == NULL || arena == NULL || arena->first_mempool == NULL) { return hdl; }

	hdl.header = head;
	hdl.addr = (void *)((char *)head + PD_HEAD_SIZE);
	hdl.handle_matrix_index = (u_int32_t)arena->table_count * TABLE_MAX_COL + arena->handle_table[arena->table_count]->
	                          entries;
	hdl.generation = 1;

	if (arena->handle_table[arena->table_count]->entries + 1 > TABLE_MAX_COL || arena->handle_table[arena->table_count]
	    ->entries == TABLE_MAX_COL) { arena->handle_table[++arena->table_count] = mempool_new_handle_table(arena); }

	Handle_Table *current_table = arena->handle_table[arena->table_count];
	current_table->handle_entries[++current_table->entries] = hdl;

	return hdl;
}


static Handle_Table *
mempool_new_handle_table(Arena *restrict arena)
{
	arena->handle_table[arena->table_count] = mempool_map_mem(PD_HDL_MATRIX_SIZE);
	if (arena->handle_table[arena->table_count] == NULL) { return NULL; }

	Handle_Table *table = arena->handle_table[arena->table_count++];
	table->entries = 0;

	return table;
}


Mempool_Header *
mempool_create_header(Mempool *restrict pool, const size_t size, const size_t offset, const u_int32_t pool_id)
{
	if ((pool->mem_size - pool->mem_offset) < (mempool_add_padding(size) + PD_HEAD_SIZE)) { goto mp_head_create_error; }
	Mempool_Header *head = NULL;

	head = (Mempool_Header *)((char *)pool->mem + offset);
	head->flags = ALLOCATED;
	head->block_size = size;
	head->next_header = NULL;
	head->previous_header = NULL;
	head->pool_id = pool_id;

	return head;

mp_head_create_error:
	printf("error: not enough room in pool for header!\n");
	fflush(stdout);
	return NULL;
}


static Mempool *
mempool_create_internal_pool(Arena *restrict arena, const size_t size)
{
	Mempool *mp = arena->first_mempool;
	if (mp->next_pool) { while (mp->next_pool) { mp = mp->next_pool; } }

	void *raw = mempool_map_mem(size + PD_POOL_SIZE);

	if (!raw) { goto mp_internal_error; }

	Mempool *new_pool = raw;
	new_pool->mem = (char *)raw + PD_POOL_SIZE;

	mp->next_pool = new_pool;
	new_pool->prev_pool = mp;

	new_pool->mem_size = size;
	new_pool->mem_offset = 0;
	new_pool->next_pool = NULL;

	arena->total_mem_size += new_pool->mem_size;

	return new_pool;

mp_internal_error:
	perror("error: failed to create internal memory pool!\n");
	fflush(stdout);
	return NULL;
}


static Mempool_Header *
mempool_find_block(Arena *arena, const size_t requested_size)
{
	if (!arena || requested_size == 0 || arena->first_mempool->mem_size == 0) { return NULL; }
	Mempool *pool = arena->first_mempool;
	u_int32_t pool_idx = 0;

	if (pool->mem_offset == 0)
	{
		// there is no first header if the offset is zero.
		return mempool_create_header(pool, requested_size, pool->mem_offset, pool_idx);
	}

	const size_t chunk_size = requested_size + PD_HEAD_SIZE;
	Mempool_Header *head = (Mempool_Header *)((char *)pool->mem);

	while (head->next_header != NULL)
	{
		if (head->flags == FREE)
		{
			if (head->block_size == requested_size || head->block_size < requested_size + PD_HEAD_SIZE) { return head; }
			if (head->block_size > requested_size + PD_HEAD_SIZE)
			{
			}
		}
	}


	//	Mempool_Header *head = (Mempool_Header *)((char *)arena->first_mempool->mem);
	//	const Mempool *pool = arena->first_mempool;
	//
	//	while (head->next_header)
	//	{
	//		if (FREE == head->flags && head->chunk_size >= requested_size)
	//		{
	//			head->flags = ALLOCATED;
	//			if (head->chunk_size == requested_size) { return head; }
	//			if (head->chunk_size > requested_size)
	//			{
	//				/* If the chunk size is bigger then the requested size, split it. */
	//			}
	//		}
	//		head = head->next_header;
	//	}
	//
	//fb_pool_create:
	//	Mempool_Header *new_head = mempool_create_header(pool, requested_size, pool->mem_offset, ALLOCATED);
	//	if (!new_head)
	//	{
	//		if (!pool->next_pool)
	//		{
	//			// Keep escalating the new size of the pool if it cant fit the allocation, until it can.
	//			size_t new_pool_size = pool->mem_size * 2;
	//			while (new_pool_size < requested_size) { new_pool_size *= 2; }
	//
	//			const Mempool *new_pool = mempool_create_internal_pool(arena, new_pool_size);
	//
	//			Mempool_Header *new_pool_head = mempool_create_header(new_pool, requested_size, 0, ALLOCATED);
	//			if (NULL == new_pool_head)
	//			{
	//				printf("mempool_find_block ERROR: new pool header failed to be created!");
	//				goto find_block_error;
	//			}
	//
	//			new_pool_head->previous_header = head;
	//			head->next_header = new_pool_head;
	//
	//			return new_pool_head;
	//		}
	//		pool = pool->next_pool;
	//		goto fb_pool_create;
	//	}
	//	return new_head;
	//
	//find_block_error:
	//	perror("mempool_find_block could not find a valid block!\n");
	//	fflush(stdout);
	//	return NULL;
}