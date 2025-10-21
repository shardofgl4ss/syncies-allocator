//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_STRUCTS_H
#define ARENA_ALLOCATOR_STRUCTS_H

#include "defs.h"
#include "types.h"
#include <stdio.h>

enum Header_Flags {
	FREE      = 0,
	ALLOCATED = 1,
	FROZEN    = 2,
};

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
	u32 pool_size;					/**< Maximum allocated size for this pool in bytes. */
	u32 pool_offset;				/**< How much space has been used so far in bytes. */
	u32 first_free_offset;
	u32 free_count;
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
 *	Flags are stored in the 3 LSB's of <size>. To get:
 *	@details
 *	flags: @code flag = header->size & 0x7; @endcode
 *	@details
 *	size: @code size = header->size >> 3; @endcode
 *	@details
 *	set flag: @code size = (size & 0x7) | ((u8)FLAG_ENUM & 0x7); @endcode
 *	@details
 *	set size: @code head->size = (u16)((new_size + PD_HEAD_SIZE) << 3); @endcode
 */
typedef struct Pool_Header {
	u32 handle_idx;			/**< Index to the header's handle.				*/
	u16 size;				/**< Size of the block in front of the header.	*/
	u16 prev_block_size;	/**< Size of the previous header block.			*/
} Pool_Header;

typedef struct Pool_Free_Header {
	u32 next_free_offset;
	u32 prev_free_offset;
	u16 size;
} Pool_Free_Header;

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
	usize block_size;		/**< Size of the block in front of the header.	*/
	usize prev_block_size;	/**< Size of the previous header block.			*/
	u32 handle_idx;	/**< Index to the header's handle.				*/
	u8 flag;			/**< Flag of the block. 0 FREE, 1 ALLOCATED, 2 FROZEN. */
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
	void *addr;					/**< address to the user's block. Equivalent to *header + sizeof(header). */
	Pool_Header *header;		/**< pointer to the sentinel header of the user's block.				  */
	u32 generation;				/**< generation of pointer, to detect stale handles and use-after-frees.  */
	u32 handle_matrix_index;	/**< flattened matrix index.											  */
} Arena_Handle;


/** @brief Row of block/header handles, or a table.
 *
 *	@details Each handle table has a max handle count of 64, allocated in chunks.
 *	Each table is in a linked list to allow easy infinite table allocation.
 *	The maximum capacity of a handle table is defined as TABLE_MAX_COL.
 *	@details
 *	To get the full size of each table, arithmetic must be done:
 *	size_t total = (TABLE_MAX_COL * PD_HANDLE_SIZE) + PD_TABLE_SIZE;
 *	@details
 *	For the bitmap, 0 == FREE/STALE, 1 == ALLOCATED.
 */
typedef struct Handle_Table {
	struct Handle_Table *next_table;
	bit64 entries_bitmap;			/**< Bitmap of used and free handles.				*/
	u32 table_id;					/**< The index of the current table.				*/
	Arena_Handle handle_entries[];	/**< array of entries via FAM. index via entries bit.	*/
} Handle_Table;


typedef struct Arena {
	Handle_Table *first_hdl_tbl;	/**< Pointer of LL of tables, matrix. */
	Memory_Pool *first_mempool;		/**< Pointer to the very first memory pool.	 */
	Memory_Pool *first_hp_pool;		/**< Pointer to the first huge page pool. */
	usize total_mem_size;			/**< The total size of all pools together.	 */
	u32 table_count;				/**< How many tables there are.*/
} Arena;

typedef struct Debug_VTable {
	void
	(*debug_print_all)(const Arena *restrict arena);
	void
	(*to_console)(void (*log_stream)(const char *restrict string, va_list arg_list), const char *restrict str, ...);
} Debug_VTable;

/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way. *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case. PH also stands for Pool Header. */
static constexpr u16 PD_ARENA_SIZE = (sizeof(Arena) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_POOL_SIZE = (sizeof(Memory_Pool) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HEAD_SIZE = (sizeof(Pool_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_FREE_PH_SIZE = (sizeof(Pool_Free_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HANDLE_SIZE = (sizeof(Arena_Handle) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_TABLE_SIZE = (sizeof(Handle_Table) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_RESERVED_F_SIZE = ((PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HDL_MATRIX_SIZE = ((((PD_HANDLE_SIZE * MAX_TABLE_HNDL_COLS) + PD_TABLE_SIZE)) + (ALIGNMENT - 1))
                                          & (u16)~(ALIGNMENT - 1);

#endif //ARENA_ALLOCATOR_STRUCTS_H
