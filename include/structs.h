//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_STRUCTS_H
#define ARENA_ALLOCATOR_STRUCTS_H

#include <stdlib.h>

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
	//u_int32_t capacity;				/**< how many handles can fit in this table.  */
	Arena_Handle handle_entries[];	/**< array of entries via FAM. index via entries. */
} Handle_Table;


typedef struct Arena {
	size_t table_count;
	Handle_Table **handle_table;	/**< Pointer of arrays to tables, 2D matrix. */
	Mempool *first_mempool;			/**< pointer to the very first memory pool.	 */
	size_t total_mem_size;			/**< the total size of all pools together.	 */
} Arena;

#endif //ARENA_ALLOCATOR_STRUCTS_H