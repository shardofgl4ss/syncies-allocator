//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_STRUCTS_H
#define ARENA_ALLOCATOR_STRUCTS_H

#include <stdlib.h>


typedef struct Memory_Slab {
} Memory_Slab;

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
typedef struct Memory_Pool {
	void *mem;						/**< Pointer to the heap region. */
	struct Memory_Pool *next_pool;	/**< Pointer to the next pool. */
	struct Memory_Pool *prev_pool;	/**< Pointer to the previous pool. */
	size_t pool_size;				/**< Maximum allocated size for this pool in bytes. */
	size_t pool_offset;				/**< How much space has been used so far in bytes. */
} Memory_Pool;

/** @brief Memory pool block header, not for huge pools.
 *
 *	@details
 *	Each header can handle a maximum of a 64KiB page, and ideally a minimum of 128 bytes.
 *	Any less than 128 bytes for an allocation and the slab should be used.
 *	Any more than 64KiB and dedicated pools shall be used.
 *	@details
 *	The next header is obtained by doing @code PD_HEAD_SIZE + head->size @endcode
 *	@details
 *	The prev header is obtained by doing @code head - (head->prev_block_size + PD_HEAD_SIZE) @endcode
 *	@details
 *	The handle index is copied over from the user handle.
 *	Valid flags are: 0 == FREE, 1 == ALLOCATED, 2 == FROZEN.
 */
typedef struct Pool_Header {
	//ptrdiff_t next_block_offs;	/**< Offset of the next header. */
	u_int16_t size;				/**< Size of the block in front of the header.	*/
	u_int16_t prev_block_size;	/**< Size of the previous header block.			*/
	u_int16_t handle_idx;		/**< Index to the header's handle.				*/
	u_int8_t flag;				/**< Flag of the block. 0 FREE, 1 ALLOCATED, 2 FROZEN. */
} Pool_Header;

typedef struct Pool_Free_Header {
	u_int32_t next_free_offset_r;
	u_int32_t prev_free_offset_r;
} Pool_Free_Header;

typedef struct Pool_Free_Header_Data {
	u_int16_t size;
	u_int16_t handle_idx;
} Pool_Free_Header_Data;

/**	@brief Extended pool header, for use in the huge page pools only.
 *
 *	@details
 *	Each header can handle a maximum of 2^63 bytes in size on 64 bit machines,
 *	if this is not enough for you, then hello future man!
 *
 *	The next header is obtained by doing PD_HEAD_SIZE + head->size.
 *	The prev header is obtained by doing head - (head->prev_block_size + PD_HEAD_SIZE)
 *	The handle index is copied over from the user handle.
 *	Valid flags are: 0 == FREE, 1 == ALLOCATED, 2 == FROZEN.
 */
typedef struct Pool_Extended_Header {
	size_t block_size;		/**< Size of the block in front of the header.	*/
	size_t prev_block_size;	/**< Size of the previous header block.			*/
	u_int32_t handle_idx;	/**< Index to the header's handle.				*/
	u_int8_t flag;			/**< Flag of the block. 0 FREE, 1 ALLOCATED, 2 FROZEN. */
} Extended_Header;

/**	@brief Arena structure for use by the user.
 *
 *	@details Each handle contains a flattened handle index for lookup. When you need to use
 *	the block address, the handle should be locked through the allocator API (*ptr = handle_lock(handle)),
 *	and then unlocked when done so defragmentation can occur.
 *
 *	@warning Each handle is given to the user to their respective block, but it should not be
 *	dereferenced manually by doing handle->addr, instead use the dereference API for safety.
 *
 *	@warning Interacting with the structure manually, beyond passing it between arena functions,
 *	is undefined behavior.
 */
typedef struct Arena_Handle {
	void *addr;						/**< address to the user's block. Equivalent to *header + sizeof(header). */
	Pool_Header *header;			/**< pointer to the sentinel header of the user's block.				  */
	u_int32_t generation;			/**< generation of pointer, to detect stale handles and use-after-frees.  */
	u_int32_t handle_matrix_index;	/**< flattened matrix index. Only for internal use.						  */
} Arena_Handle;


/** @brief Row of block/header handles, or a table.
 *
 *	@details Each handle table has a max handle count of 64, allocated in chunks.
 *	Each table is in a linked list to allow easy infinite table allocation.
 *	The maximum capacity of a handle table is defined as TABLE_MAX_COL.
 *	To get the full size of each table, arithmetic must be done:
 *	size_t total = (TABLE_MAX_COL * PD_HANDLE_SIZE) + PD_TABLE_SIZE;
 */
typedef struct Handle_Table {
	struct Handle_Table *next_table;
	u_int64_t entries_bitmap;		/**< Bitmap of used and free handles.				*/
	u_int16_t table_id;				/**< The index of the current table.				*/
	Arena_Handle handle_entries[];	/**< array of entries via FAM. index via entries bit.	*/
} Handle_Table;


typedef struct Arena {
	u_int32_t table_count;		/**< How many tables there are.*/
	Handle_Table *first_hdl_tbl;	/**< Pointer of LL of tables, matrix. */
	Memory_Pool *first_mempool;	/**< Pointer to the very first memory pool.	 */
	Memory_Pool *first_hp_pool; /**< Pointer to the first huge page pool. */
	size_t total_mem_size;		/**< The total size of all pools together.	 */
} Arena;

#endif //ARENA_ALLOCATOR_STRUCTS_H