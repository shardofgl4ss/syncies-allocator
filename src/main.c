#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

/* Needs to be in multiples of 8.
 * An alignment of 16 however, is optimal for
 * SIMD instructions, while 32 is optimal for
 * AVX512 instructions. So it is left to the
 * user to decide.
 */
#ifndef ALIGNMENT
	#define	ALIGNMENT	8
#endif

#ifndef FIRST_POOL_ALLOC
	#define FIRST_POOL_ALLOC	16384
#endif

#ifndef MINIMUM_POOL_BLOCK_ALLOC
	#define MINIMUM_POOL_BLOCK_ALLOC	8
#endif

#define TABLE_MAX_COL	64


/**
 * @enum Header_Block_Flags
 * @brief The type of each block trailing a header.
 * 0 = IN_USE, 1 = FREE
 *
 * @details When a new header is created, it first is set to FREE,
 * so mempool_find_block can properly identify a free-to-use memory block.
 * The same happens if the user frees a pointer in use, so the space can
 * be reused later.
 */
typedef enum Header_Block_Flags {
	ALLOCATED = 0,
	FREE      = 1,
	FROZEN    = 2,
} Header_Block_Flags;


/** @brief Memory pool data structure.
 *
 *	@details
 *	Every pool is linked to the next and the next are linked to the previous,
 *	resulting in a linked-list style of arena. total_mem_size represents
 *	the size of all memory pools together (only in the first pool),
 *	while mem_size is the maximum size for each.
 *
 *	Destroying all memory pools requires both *next_pool and *prev_pool,
 *	as it requires walking forward through all pools, then walking back,
 *	and freeing last-to-first.
 */
typedef struct Mempool {
	void *mem;					/**< Pointer to the heap region. */
	size_t mem_size;			/**< Maximum allocated size for this pool in bytes. */
	size_t mem_offset;			/**< How much space has been used so far in bytes. */
	struct Mempool *next_pool;	/**< Pointer to the next pool. */
	struct Mempool *prev_pool;	/**< Pointer to the previous pool. */
} Mempool;


/** @brief Memory pool block header.
 *
 *	@details
 *	Every allocation onto the arena for a new spot in the heap pool
 *	will create a new header to manage that block.
 *	This allows using the user's own pointer as an identifier
 *	for the header that the pointer's block header is responsible for.
 *
 *	This also has a linked-list style, linking previous and next headers,
 *	for fast indexing, but is more-so to be able to coalesce multiple
 *	free blocks into a single one, reducing fragmentation of the arena.
 *
 *	@note @param flags can only have the values:
 *	0 NOT_FREE
 *	1 FREE
 *	2 FROZEN
 */
typedef struct Mempool_Header {
	size_t chunk_size;						/**< Size of the header and block. */
	u_int32_t pool_id;						/**< ID of the current pool. Used to detect cross-pool linkage. */
	Header_Block_Flags flags;				/**< header-block flag. */
	struct Mempool_Header *next_header;		/**< pointer to the next block in the chain. */
	struct Mempool_Header *previous_header; /**< pointer to the previous block in the chain.*/
} Mempool_Header;


/**	@brief Arena structure for use by the user.
 *
 *	@details Each handle contains a flattened handle index for lookup. When you need to use
 *	the block address, the handle should be pinned through the allocator API (arena_pin(handle)),
 *	and then unpinned when done so defragmentation can occur.
 *
 *	@warning Each handle is given to the user to their respective block, but it should not be
 *	dereferenced manually by doing handle->addr, instead use the dereference API for safety.
 *
 *	@warning Interacting with the structure manually, beyond passing it between arena functions,
 *	is undefined behavior.
 *
 */
typedef struct Arena_Handle {
	void *addr;						/**< address to the user's block. Equivalent to *header + sizeof(header). */
	Mempool_Header *header;			/**< pointer to the sentinel header of the user's block.				  */
	u_int32_t generation;			/**< generation of pointer, to detect stale handles and use-after-frees.  */
	u_int32_t handle_matrix_index;	/**< flattened matrix index. Only for internal use.						  */
} Arena_Handle;


/** @brief Row of block/header handles, or a table.
 *
 * @details Each handle table has a max handle count of:
 * 4096 / (sizeof(Arena_Handle) + sizeof(Mempool_Header) + MIN_POOL_BLOCK_ALLOC)
 * which equates to about 64 handles per row.
 */
typedef struct Handle_Table {
	u_int32_t entries;				/**< how many handles there are in this table.	  */
	u_int32_t capacity;				/**< how many handles can fit in this table.      */
	Arena_Handle handle_entries[];	/**< array of entries via FAM. index via entries. */
} Handle_Table;


typedef struct Arena {
	size_t table_count;
	Handle_Table **handle_table;	/**< Pointer of arrays to tables, 2D matrix. */
	Mempool *first_mempool;			/**< pointer to the very first memory pool.	 */
	size_t total_mem_size;			/**< the total size of all pools together.	 */
} Arena;


inline static size_t
mempool_add_padding(const size_t input) { return (input + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1); }


/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way. *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case.							  */
#define PD_ARENA_SIZE		((sizeof(Arena) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1))
#define PD_POOL_SIZE		((sizeof(Mempool) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1))
#define PD_HEAD_SIZE		((sizeof(Mempool_Header) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1))
#define PD_HANDLE_SIZE		((sizeof(Arena_Handle) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1))
#define PD_TABLE_SIZE		((sizeof(Handle_Table) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1))
#define PD_RESERVED_F_SIZE	((PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1) & (size_t)~(ALIGNMENT - 1))
#define PD_HDL_MATRIX_SIZE	(((PD_HANDLE_SIZE * TABLE_MAX_COL) + PD_TABLE_SIZE) + (ALIGNMENT - 1) & (size_t)~(ALIGNMENT - 1))


inline static void *
mempool_map_mem(const size_t bytes)
{
	return mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}


static Handle_Table *
mempool_new_handle_table(Arena *restrict arena)
{
	arena->handle_table[arena->table_count] = mempool_map_mem(ALIGNED_HDL_TBL_SIZE);
	if (arena->handle_table[arena->table_count] == NULL) { return NULL; }

	Handle_Table *table = arena->handle_table[arena->table_count++];
	table->capacity = TABLE_MAX_COL;
	table->entries = 0;

	return table;
}


static Arena_Handle
mempool_create_handle_and_entry(const Arena *restrict arena, Mempool_Header *restrict head)
{
	Arena_Handle hdl;

	hdl.header = head;
	hdl.addr = (void *)((char *)head + ALIGNED_HEAD_SIZE);
	hdl.handle_matrix_index = (u_int32_t)arena->table_count * TABLE_MAX_COL + arena->handle_table[arena->table_count]->
	                          entries;
	hdl.generation = 1;

	return hdl;
}


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
	void *raw_pool = mempool_map_mem(FIRST_POOL_ALLOC + ALIGNED_RESERVED_F_SIZE);

	if (raw_pool == MAP_FAILED) { goto mp_create_error; }

	/*	Add the reserved_space directly to the mapping, then make the starting first_pool->mem		*
	 *	after the reserved space, then just set mem_size to the FIRST_ARENA_POOL_SIZE.				*
	 *	This spoofs a max memory of 4096 when its actually 4096 + reserved.							*
	 *  Alignment is added to Mempool in case the user changes it to 16 or 32, it will be aligned.	*
	 *  The alignment however adds some complexity later on.										*/

	Arena *arena = raw_pool;
	Mempool *first_pool = (Mempool *)((char *)raw_pool + sizeof(Arena));

	first_pool->mem = (char *)raw_pool + ALIGNED_RESERVED_F_SIZE;
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


static inline void
mp_destroy(void *restrict mem, const size_t bytes)
{
	static int index = 0;
	const int x = munmap(mem, bytes);

	if (x == -1) { goto destroy_fail; }

	printf("pool %d: destroyed\n", ++index);
	return;

destroy_fail:
	perror("error: arena destruction failure! this should never happen, there is probably lost memory!\n");
	fflush(stdout);
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
		mp_destroy(mp->mem, mp->mem_size + ALIGNED_RESERVED_F_SIZE);
		printf("arena destroyed\n");
		fflush(stdout);
	}

	mp = mp->next_pool;
	mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + ALIGNED_RESERVED_F_SIZE);

	while (mp->next_pool)
	{
		mp = mp->next_pool;
		mp_destroy(mp->prev_pool->mem, mp->prev_pool->mem_size + ALIGNED_POOL_SIZE);
	}

	mp_destroy(mp->mem, mp->mem_size + ALIGNED_POOL_SIZE);

	printf("arena destroyed\n");
	fflush(stdout);
}


/**	@brief Creates a new block header
 *
 *	@details Flag options: 0 = NOT_FREE, 1 = FREE, 2 = RESERVED
 *
 *	@param pool Pointer to the pool to create the header in.
 *	@param size Size of the block and header.
 *	@param offset Offset to the base of the pool.
 *	@param flag The flag to give the header.
 *	@return a valid block header pointer.
 *
 *	@note Does not link headers.
 */
Mempool_Header *
mempool_create_header(const Mempool *pool, const size_t size, const size_t offset, const Header_Block_Flags flag)
{
	if ((pool->mem_size - pool->mem_offset) < (mempool_add_padding(size) + ALIGNED_HEAD_SIZE))
	{
		goto mp_head_create_error;
	}
	Mempool_Header *head = NULL;

	head = (Mempool_Header *)((char *)pool->mem + offset);
	if (!head) { goto mp_head_create_error; }
	head->flags = flag;
	head->chunk_size = size;
	head->next_header = NULL;
	head->previous_header = NULL;

	return head;

mp_head_create_error:
	printf("error: not enough room in pool for header!\n");
	fflush(stdout);
	return NULL;
}


/** @brief Creates a new memory pool.
 *	@param arena Pointer to the arena to create a new pool in.
 *	@param size How many bytes to give to the new pool.
 *	@return	Returns a pointer to the new memory pool.
 *
 *	@note Should not be used by the user, only by the allocator.
 *	Handles linking pools, and updating the total_mem_size in the first pool.
 *	@warning Returns NULL if allocating a new pool fails, or if provided size is zero.
 */
static Mempool *
mempool_create_internal_pool(Arena *restrict arena, const size_t size)
{
	Mempool *mp = arena->first_mempool;
	if (mp->next_pool) { while (mp->next_pool) { mp = mp->next_pool; } }

	void *raw = mempool_map_mem(size + ALIGNED_POOL_SIZE);

	if (MAP_FAILED == raw) { goto mp_internal_error; }

	Mempool *new_pool = raw;
	new_pool->mem = (char *)raw + ALIGNED_POOL_SIZE;

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


/** @brief Analyzes the header-block chain, making a new header if there is no free one to use.
 *	If there is no space at all in the pool, it will create a new pool for it.
 *	@param arena The given arena to find a block in.
 *	@param requested_size User-requested size, aligned by ALIGNMENT.
 *	@return header of a valid block, if creating one was successful.
 *
 *	@warning Will return NULL if a new pool's header could not be made, which should be impossible.
 */
static Mempool_Header *
mempool_find_block(Arena *arena, const size_t requested_size)
{
	Mempool_Header *head = (Mempool_Header *)((char *)arena->first_mempool->mem);
	const Mempool *pool = arena->first_mempool;

	while (head->next_header)
	{
		if (FREE == head->flags && head->chunk_size >= requested_size)
		{
			head->flags = ALLOCATED;
			if (head->chunk_size == requested_size) { return head; }
			if (head->chunk_size > requested_size)
			{
				/* If the chunk size is bigger then the requested size, split it. */
			}
		}
		head = head->next_header;
	}

fb_pool_create:
	Mempool_Header *new_head = mempool_create_header(pool, requested_size, pool->mem_offset, ALLOCATED);
	if (!new_head)
	{
		if (!pool->next_pool)
		{
			// Keep escalating the new size of the pool if it cant fit the allocation, until it can.
			size_t new_pool_size = pool->mem_size * 2;
			while (new_pool_size < requested_size) { new_pool_size *= 2; }

			const Mempool *new_pool = mempool_create_internal_pool(arena, new_pool_size);

			Mempool_Header *new_pool_head = mempool_create_header(new_pool, requested_size, 0, ALLOCATED);
			if (NULL == new_pool_head)
			{
				printf("mempool_find_block ERROR: new pool header failed to be created!");
				goto find_block_error;
			}

			new_pool_head->previous_header = head;
			head->next_header = new_pool_head;

			return new_pool_head;
		}
		pool = pool->next_pool;
		goto fb_pool_create;
	}
	return new_head;

find_block_error:
	perror("mempool_find_block could not find a valid block!\n");
	fflush(stdout);
	return NULL;
}


/**	@brief Clears up defragmentation of the memory pool where there is any.
 *
 *	@details Indexes through the memory pool from first to last,
 *	if there are two adjacent blocks that are free, links the first free
 *	to the next non-free, then updates head->chunk_size of the first free.
 *
 *	@param head Pointer to a header to index through. Can be any header.
 */
static void
mempool_defragment(Arena *arena)
{
}


/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
inline static void *
return_vptr(Mempool_Header *head) { return (char *)head + ALIGNED_HEAD_SIZE; }


/** @brief Allocates a new block of memory.
 *
 *	@param arena Pointer to the arena to work on.
 *	@param size How many bytes the user requests.
 *	@return arena handle to the user, use instead of a vptr.
 *
 *	@note All size is rounded up to the nearest value of ALIGNMENT, and a minimum valid size is 8 bytes.
 *	@warning If a size of zero is provided or a sub-function fails, NULL is returned.
 */
Arena_Handle *
arena_alloc(Arena *arena, const size_t size)
{
	if (arena == NULL || size == 0 || arena->first_mempool == NULL) { return NULL; }

	Mempool *first_pool = arena->first_mempool;
	const size_t input_bytes = mempool_add_padding(size);
	Mempool_Header *head = NULL;

	head = mempool_find_block(arena, input_bytes);
	if (!head) { return NULL; }
	return return_vptr(head);
}


inline static bool
mempool_handle_generation_lookup(const Arena *arena, const Arena_Handle *user_handle)
{
	const size_t row_index = user_handle->handle_matrix_index / TABLE_MAX_COL;
	const size_t col_index = user_handle->handle_matrix_index % TABLE_MAX_COL;

	if (arena->handle_table[row_index]->handle_entries[col_index].generation != user_handle->generation)
	{
		return false;
	}
	return true;
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
				munmap(pool->prev_pool->mem, pool->prev_pool->mem_size + ALIGNED_POOL_SIZE);
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


inline static Arena *
return_base_arena(const Arena_Handle *user_handle)
{
	Mempool_Header *head = user_handle->header;

	if (head->previous_header) { while (head->previous_header) { head = head->previous_header; } }
	return (Arena *)((char *)head - (ALIGNED_POOL_SIZE + mempool_add_padding(sizeof(Arena))));
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

	const size_t header_bytes = head_count * mempool_add_padding(sizeof(Mempool_Header));
	size_t handle_count = 0;

	for (size_t i = 0; i < arena->table_count; i++) { handle_count += arena->handle_table[i]->entries; }

	const size_t handle_bytes = handle_count * sizeof(Arena_Handle);
	const size_t reserved_mem = handle_bytes + header_bytes + (pool_count * sizeof(Mempool)) + sizeof(Arena);

	printf("Struct memory of: headers: %lu\n", header_bytes);
	printf("Struct memory of: handles: %lu\n", handle_bytes);
	printf("Struct memory of: pools: %lu\n", pool_count * sizeof(Mempool));
	printf("Total reserved memory: %lu\n", reserved_mem);
	printf("Total memory used: %lu\n", total_pool_mem + reserved_mem);
	fflush(stdout);
}


int
main(void)
{
	Arena *arena = arena_create();
	if (!arena) return 0;

	char *test = arena_alloc(arena, 64);
	if (!test) return 0;


	fgets(test, 64, stdin);

	printf("%s\n", test);
	arena_destroy(arena);
}