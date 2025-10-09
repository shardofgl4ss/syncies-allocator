//
// Created by SyncShard on 10/9/25.
//

#include "../include/alloc_lib.h"


/**	@brief Creates a dynamic, automatically resizing arena with base size of 16KiB.
 *
 *	Example: @code Arena *arena = arena_create(); @endcode
 *
 *	@return Pointer to the very first memory pool, required for indexing.
 *
 *	@warning If heap allocation fails, NULL is returned.
 *	The user should also never interact with the Mempool struct
 *	beyond storing and passing it.
 */
Arena *
arena_create()
{
	void *raw_pool = mempool_map_mem(FIRST_POOL_ALLOC + PD_RESERVED_F_SIZE);

	if (raw_pool == MAP_FAILED) { goto mp_create_error; }

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set mem_size to the FIRST_ARENA_POOL_SIZE.				*
	 *	This spoofs a max memory of 4096 when its actually 4096 + reserved.							*
	 *  Alignment is added to Mempool in case the user changes it to 16 or 32, it will be aligned.	*
	 *  The alignment however adds some complexity later on.										*/

	Arena *arena = raw_pool;
	Mempool *first_pool = (Mempool *)((char *)raw_pool + sizeof(Arena));

	first_pool->mem = (char *)raw_pool + PD_RESERVED_F_SIZE;
	first_pool->mem_offset = 0;
	first_pool->mem_size = FIRST_POOL_ALLOC;
	first_pool->next_pool = NULL;
	first_pool->prev_pool = NULL;

	arena->total_mem_size = FIRST_POOL_ALLOC;
	arena->table_count = 0;
	arena->handle_table[arena->table_count++] = mempool_new_handle_table(arena);

	return arena;

mp_create_error:
	perror("ERR_NO_MEMORY: arena creation failed!\n");
	fflush(stdout);
	return NULL;
}


/// @brief Destroys a whole arena, deallocating it and setting all values to NULL or 0
/// @param arena The arena to destroy
void
arena_destroy(Arena *restrict arena)
{
	if (!arena) { return; }
	const Mempool *mp = arena->first_mempool;

	if (!mp->next_pool)
	{
		mp_destroy(mp->mem, mp->mem_size + PD_RESERVED_F_SIZE);
		printf("arena destroyed\n");
		fflush(stdout);
	}

	mp = mp->next_pool;
	mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + PD_RESERVED_F_SIZE);

	while (mp->next_pool)
	{
		mp = mp->next_pool;
		mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + PD_POOL_SIZE);
	}

	mp_destroy(mp->mem, mp->mem_size + PD_POOL_SIZE);

	printf("arena destroyed\n");
	fflush(stdout);
}


/**	@brief Clears up defragmentation of the memory pool where there is any.
 *
 *	@details Indexes through the memory pool from first to last,
 *	if there are two adjacent blocks that are free, links the first free
 *	to the next non-free, then updates head->chunk_size of the first free.
 *
 *	@param arena Arena to defragment.
 */
static void
arena_defragment(Arena *arena)
{
	// wip
}


/** @brief Allocates a new block of memory.
 *
 *	@param arena Pointer to the arena to work on.
 *	@param size How many bytes the user requests.
 *	@return arena handle to the user, use instead of a vptr.
 *
 *	@note All size is rounded up to the nearest value of ALIGNMENT, and a minimum valid size is 8 bytes.
 *	@warning If a size of zero is provided or a sub-function fails, NULL is returned.
 */
Arena_Handle
arena_alloc(Arena *arena, const size_t size)
{
	if (arena == NULL || size == 0 || arena->first_mempool == NULL) { return NULL; }

	Arena_Handle user_handle = {
		.addr = NULL,
		.generation = 0,
		.handle_matrix_index = 0,
		.header = NULL,
	};

	const size_t input_bytes = mempool_add_padding(size);
	Mempool_Header *head = NULL;

	head = mempool_find_block(arena, input_bytes);
	if (!head) { return user_handle; }
	user_handle = mempool_create_handle_and_entry(arena, head);
	if (user_handle.addr != NULL) { return user_handle; }

	perror("error: invalid handle was given! things will break!\n");
	return user_handle;
}


/**
 * @brief Clears all pools, deleting all but the first pool, performing a full reset, unless 1 is given.
 * @param arena The arena to reset.
 * @param reset_type 0: Will full reset the entire arena.
 * 1: Will soft reset the arena, not deallocating excess pools.
 * 2: Will do the same as 0, but will not wipe the first arena.
 */
void
arena_reset(const Arena *restrict arena, const int reset_type)
{
	Mempool *restrict pool = arena->first_mempool;

	switch (reset_type)
	{
		case 1:
		mp_soft_reset:
			pool->mem_offset = 0;
			return;
		case 0:
			pool->mem_offset = 0;
		case 2:
			if (!pool->next_pool) { return; }

			while (pool->next_pool)
			{
				pool = pool->next_pool;
				munmap(pool->prev_pool->mem, pool->prev_pool->mem_size + PD_POOL_SIZE);
				pool->prev_pool->next_pool = NULL;
				pool->prev_pool = NULL;
			}
			arena->first_mempool->mem = (char *)0;
			break;
		default:
			printf("debug: unknown arena_reset() type!\nDefaulting to soft reset!\n");
			goto mp_soft_reset;
	}
}


void
arena_free(Arena_Handle *user_handle)
{
	Mempool_Header *head = user_handle->header;

	const Arena *arena = return_base_arena(user_handle);

	if (!mempool_handle_generation_lookup(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return;
	}

	user_handle->header->flags = 1;
	user_handle->generation++;
	user_handle->addr = NULL;
}


//inline static void
//arena_freeze_handle(const Arena_Handle *handle) { handle->header->flags = FROZEN; }


//Arena_Handle
//arena_reallocate(Arena_Handle *user_handle, const size_t size)
//{
//}


void *
arena_deref(Arena_Handle *user_handle)
{
	const Arena *arena = return_base_arena(user_handle);

	if (!mempool_handle_generation_lookup(arena, user_handle))
	{
		perror("error: stale handle detected!\n");
		return NULL;
	}

	user_handle->header->flags = FROZEN;
	user_handle->addr = (void *)((char *)user_handle->addr + mempool_add_padding(sizeof(Mempool_Header)));
}


void
arena_debug_print_memory_usage(const Arena *arena)
{
	const Mempool *mempool = arena->first_mempool;
	const Mempool_Header *header = (Mempool_Header *)arena->first_mempool->mem;

	u_int32_t pool_count = 0, head_count = 0;
	size_t total_pool_mem = mempool->mem_size;

	while (mempool)
	{
		total_pool_mem += mempool->mem_size;
		mempool = mempool->next_pool;
		pool_count++;
	}
	while (header)
	{
		header = header->next_header;
		head_count++;
	}

	printf("Total amount of pools: %d\n", pool_count);
	printf("Pool memory (B): %lu\n", total_pool_mem);

	const size_t header_bytes = head_count * PD_HEAD_SIZE;
	size_t handle_count = 0;

	for (size_t i = 0; i < arena->table_count; i++) { handle_count += arena->handle_table[i]->entries; }

	const size_t handle_bytes = handle_count * PD_HANDLE_SIZE;
	const size_t reserved_mem = handle_bytes + header_bytes + (pool_count * PD_POOL_SIZE) + PD_ARENA_SIZE;

	printf("Struct memory of: headers: %lu\n", header_bytes);
	printf("Struct memory of: handles: %lu\n", handle_bytes);
	printf("Struct memory of: pools: %lu\n", pool_count * PD_POOL_SIZE);
	printf("Total reserved memory: %lu\n", reserved_mem);
	printf("Total memory used: %lu\n", total_pool_mem + reserved_mem);
	fflush(stdout);
}
