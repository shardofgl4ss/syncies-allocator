//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_STRUCTS_H
#define ARENA_ALLOCATOR_STRUCTS_H

#include "defs.h"
#include "types.h"
#include <stdio.h>


/**	@brief Memory Pool header-block flags.
 *	Each value is in a bitmap to be able to store each in a single 8-bit integer.
 *
 *	@details
 *	To write flags:				@code x |= FLAG; @endcode
 *	@details
 *	To write multiple flags:	@code x |= FLAG1 | FLAG2; @endcode
 *	@details
 *	To read flags:				@code x & FLAG; @endcode
 *	@details
 *	To toggle a flag:			@code x ^= FLAG; @endcode
 *	@details
 *	To clear a flag:			@code x &= ~FLAG; @endcode
 *	@details
 *	To clear multiple flags:	@code x &= ~(FLAG1 | FLAG2); @endcode
 */
enum Header_Flags : uint {
	PH_FREE        = 1 << 0,	/**< FREE: Empty block. */
	PH_ALLOCATED   = 1 << 1,	/**< ALLOCATED: free to relocate and defrag. */
	PH_FROZEN      = 1 << 2,	/**< FROZEN: dictates that the handle has been dereferenced, do NOT touch. */
	PH_SENTINEL_F  = 1 << 3,	/**< SENTINEL: Marks the very first or last header in the pool. */
	PH_SENTINEL_L  = 1 << 4,	/**< SENTINEL: Marks the very first or last header in the pool. */
	PH_PREV_FREE   = 1 << 5,	/**< PREV_FREE and NEXT_FREE flags are entirely just to help coalesce free blocks. */
	PH_NEXT_FREE   = 1 << 6,	/**< PREV_FREE and NEXT_FREE flags are entirely just to help coalesce free blocks. */
	PH_HUGE_PAGE   = 1 << 7,	/**< HUGE_PAGE: Determines if this block is in the huge page pool or not. */
	PH_ZEROED      = 1 << 8,	/**< ZEROED: for future calloc like impl. */
	PH_SENSITIVE   = 1 << 9,	/**< SENSITIVE: for heap memory that needs to be zeroed out before reuse. */
	PH_RECENT_FREE = 1 << 10,	/**< RECENT_FREE: for later hardening, might not use, but hey, still got a ton more bits. */
};


/** @brief Memory pool block header, not for huge pools or slabs, but right in the middle.
 *
 *	@details
 *	Each header can handle a maximum of a 128KiB page, and ideally a minimum of 128 bytes.
 *	Any less than 128 bytes for an allocation and the slab should be used.
 *	Any more than 128KiB and dedicated pools shall be used.
 *	@details
 *	The next header is obtained by doing @code PD_HEAD_SIZE + head->size @endcode
 *	@details
 *	The prev header is obtained by doing @code head - (head->prev_block_size + PD_HEAD_SIZE) @endcode
 *	The handle index is copied over from the user handle.
 *
 *	@details
 *	The block_flag variable is an 8-bit bitmap corresponding to the Header_Flags flag types. Operations for bitmaps:
 *	@details
 *	To write flags:				@code block_flag |= FLAG; @endcode
 *	@details
 *	To write multiple flags:	@code block_flag |= FLAG1 | FLAG2; @endcode
 *	@details
 *	To read flags:				@code block_flag & FLAG; @endcode
 *	@details
 *	To toggle a flag:			@code block_flag ^= FLAG; @endcode
 *	@details
 *	To clear a flag:			@code block_flag &= ~FLAG; @endcode
 *	@details
 *	To clear multiple flags:	@code block_flag &= ~(FLAG1 | FLAG2); @endcode
 */
typedef struct Pool_Header {
	u32 handle_idx;			/**< Index to the header's handle.				*/
	u32 size;				/**< Size of the block in front of the header.	*/
	u32 prev_block_size;	/**< Size of the previous header block.			*/
	bit32 block_flags;		/**< Enum bitmap for flags.						*/
} Pool_Header;


/** @brief Freed memory pool block header, not for huge pools or slabs, but right in the middle.
 *
 *	@details
 *	The offset of each variable and header for both normal and freed headers,
 *	makes flags persist through casting between free and non-free. However,
 *	other variables will be broken, so it is a requirement to make a temporary
 *	buffer to store allocated_head->size.
 *	@details
 *	see Pool_Header for more details.
 */
typedef struct Pool_Free_Header {
	struct Pool_Free_Header *next_free;
	u32 size;
	bit32 block_flags;
} Pool_Free_Header;


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
	void *mem;						/**< Pointer to the heap region.					*/
	struct Memory_Pool *next_pool;	/**< Pointer to the next pool.						*/
	struct Memory_Pool *prev_pool;	/**< Pointer to the previous pool.					*/
	Pool_Free_Header *first_free;	/**< Pointer to the first freed header.				*/
	u32 pool_size;					/**< Maximum allocated size for this pool in bytes. */
	u32 pool_offset;				/**< How much space has been used so far in bytes.	*/
	u32 free_count;					/**< How many freed headers there are in this pool. */
} Memory_Pool;


/**	@brief Extended pool header, for use in the huge page pools only.
 *
 *	@details
 *	Each header can handle a maximum of 2^63 bytes in size on 64 bit machines,
 *	if this is not enough for you, then hello future man!
 *
 *	The next header is obtained by doing PD_HEAD_SIZE + head->size.
 *	The prev header is obtained by doing head - (head->prev_block_size + PD_HEAD_SIZE)
 *	The handle index is copied over from the user handle.
 *	Refer to the normal header struct doc for more information.
 */
typedef struct Pool_Extended_Header {
	usize block_size;		/**< Size of the block in front of the header.	*/
	usize prev_block_size;	/**< Size of the previous header block.			*/
	u32 handle_idx;			/**< Index to the header's handle.				*/
	bit32 block_flags;		/**< Flag-enum bitmap of the header.			*/
} Extended_Header;


/**	@brief Arena structure for use by the user.
 *
 *	@details Each handle contains a flattened handle index for lookup. When you need to use
 *	the block address, the handle should be locked through the allocator API (*ptr = handle_lock(handle)),
 *	and then unlocked when done so defragmentation can occur.
 *
 *	@note if generation is equal to the maximum number for an unsigned 32 bit, it will be treated
 *	as if it was an invalid handle.
 *
 *	@warning Each handle is given to the user to their respective block, but it should not be
 *	dereferenced manually by doing handle->addr, instead use the dereference API for safety.
 *
 *	@warning Interacting with the structure manually, beyond passing it between arena functions,
 *	is undefined behavior.
 */
struct Arena_Handle {
	void *addr;					/**< address to the user's block. Equivalent to *header + sizeof(header). */
	Pool_Header *header;		/**< pointer to the sentinel header of the user's block.				  */
	u32 generation;				/**< generation of pointer, to detect stale handles and use-after-frees.  */
	u32 handle_matrix_index;	/**< flattened matrix index.											  */
};

/** @brief Row of block/header handles, or a tablex.
 *
 *	@details Each handle table has a max handle count of 64, allocated in chunks.
 *	Each table is in a linked list to allow easy infinite table allocation.
 *	The maximum capacity of a handle table is defined as TABLE_MAX_COL.
 *	@details
 *	To get the full size of each table, arithmetic must be done:
 *	size_t total = (TABLE_MAX_COL * PD_HANDLE_SIZE) + PD_TABLE_SIZE;
 *	@details
 *	For the bitmap, 0 == FREE, 1 == ALLOCATED.
 */
typedef struct Handle_Table {
	struct Handle_Table *next_table;
	bit64 entries_bitmap;			/**< Bitmap of used and free handles.							*/
	u32 table_id;					/**< The index of the current table.							*/
	struct Arena_Handle handle_entries[];	/**< array of entries via FAM. index via entries bit.	*/
} Handle_Table;


typedef struct Arena {
	Handle_Table *first_hdl_tbl;	/**< Pointer of LL of tables, matrix.		*/
	Memory_Pool *first_mempool;		/**< Pointer to the first memory pool.		*/
	Memory_Pool *first_hp_pool;		/**< Pointer to the first huge page pool.	*/
	usize total_mem_size;			/**< The total size of all pools together.	*/
	u32 table_count;				/**< How many tables there are.				*/
} Arena;

typedef struct Debug_VTable {
	void
	(*debug_print_all)();
	void
	(*to_console)(void (*log_stream)(const char *restrict string, va_list arg_list), const char *restrict str, ...);
} Debug_VTable;

/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way.	   *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case. PH also stands for Pool Header. */
static constexpr u16 PD_ARENA_SIZE = (sizeof(Arena) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_POOL_SIZE = (sizeof(Memory_Pool) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HEAD_SIZE = (sizeof(Pool_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_FREE_PH_SIZE = (sizeof(Pool_Free_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HANDLE_SIZE = (sizeof(struct Arena_Handle) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_TABLE_SIZE = (sizeof(Handle_Table) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_RESERVED_F_SIZE = ((PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HDL_MATRIX_SIZE = ((((PD_HANDLE_SIZE * MAX_TABLE_HNDL_COLS) + PD_TABLE_SIZE)) + (ALIGNMENT - 1))
                                          & (u16)~(ALIGNMENT - 1);

#endif //ARENA_ALLOCATOR_STRUCTS_H
