//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_STRUCTS_H
#define ARENA_ALLOCATOR_STRUCTS_H

#include "defs.h"
#include "types.h"
#include <stdio.h>

// clang-format off

/**
 * 	Memory Pool header-block flags.
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
enum Header_Flags : u16 {
	F_FREE        = (1 << 0),	/**< FREE: Empty block.									*/
	F_ALLOCATED   = (1 << 1),	/**< ALLOCATED: free to relocate and defrag.						*/
	F_FROZEN      = (1 << 2),	/**< FROZEN: dictates that the handle has been dereferenced, do NOT touch.		*/
	F_FIRST_HEAD  = (1 << 3),	/**< SENTINEL: Marks the very first header in the pool.					*/
	F_SENTINEL    = (1 << 4),	/**< SENTINEL: Marks the very last header in the pool.					*/
	F_PREV_FREE   = (1 << 5),	/**< PREV_FREE and NEXT_FREE flags are entirely just to help coalesce free blocks.	*/
	F_NEXT_FREE   = (1 << 6),	/**< PREV_FREE and NEXT_FREE flags are entirely just to help coalesce free blocks.	*/
	F_ZEROED      = (1 << 7),	/**< ZEROED: for future calloc like impl.						*/
	F_SENSITIVE   = (1 << 8),	/**< SENSITIVE: for heap memory that needs to be zeroed out before reuse.		*/
	F_RECENT_FREE = (1 << 9),	/**< RECENT_FREE: for later hardening, might not use.					*/
	F_RAW         = (1 << 10),	/**< RAW_TYPE: No handle is associated with this header, only a raw void ptr.		*/
	F_HUGE_PAGE   = (1 << 11),	/**< HUGE_PAGE: Determines if this block is in the huge page pool or not.		*/
	F_SLAB_BLOCK  = (1 << 12),	/**< SLAB_BLOCK: Determines if this flag marks a slab or not.				*/
};


/**
 * 	Memory pool block header, not for huge pools or slabs, but right in the middle.
 *
 *	@details
 *	Each header can handle a maximum of a 128KiB page, and ideally a minimum of 128 bytes.
 *	Any less than 128 bytes for an allocation and the slab (WIP) should be used.
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
 *	To write flags:			@code bitflags |= FLAG; @endcode
 *	@details
 *	To write multiple flags:	@code bitflags |= FLAG1 | FLAG2; @endcode
 *	@details
 *	To read flags:			@code bitflags & FLAG; @endcode
 *	@details
 *	To toggle a flag:		@code bitflags ^= FLAG; @endcode
 *	@details
 *	To clear a flag:		@code bitflags &= ~FLAG; @endcode
 *	@details
 *	To clear multiple flags:	@code bitflags &= ~(FLAG1 | FLAG2); @endcode
 *
 * 	@note
 * 	The pool header could easily be reduced down to 8 bytes, however, this
 * 	would break preservation of the second and third members of the struct
 *	during casting, and add extra complexity of tracking the true block size,
 *	and on top of that, would also probably add unfixable fragmentation.
 */
typedef struct Pool_Header {
	// handle_idx is kept for compatibility with casting to Pool_Free_Node if SYN_USE_RAW is set.
	u32 handle_idx;		/**< Index to the header's handle.		*/
	u32 allocation_size;	/**< Actual size of the allocation.		*/
	u32 chunk_size;		/**< Size of the entire chunk, with header.	*/
	bit32 bitflags;		/**< Enum bitmap for flags.			*/
} __attribute__((aligned(16))) pool_header_t;

/**
 * 	Freed memory pool block header. in a singly-linked-list style.
 *
 * 	@details
 * 	Casting a header to and from pool_free_header_t and pool_header_t,
 * 	will preserve size and bitflags, but not preserve pool_free_node_t's 
 * 	next_free or pool_header_t's handle_idx.
 *
 *	@details
 *	see Pool_Header for more details.
 */
typedef struct Pool_Free_Node {
	struct Pool_Free_Node *next_free;
	u32 size;
	bit32 bitflags;
} __attribute__((aligned(16))) pool_free_node_t;

/**
 * 	Extended pool header, for use in the huge page pools only.
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
typedef struct Pool_Header_Ext {
	u64 size;		/**< Size of the block in front of the header.	*/
	u64 prev_block_size;	/**< Size of the previous header block.		*/
	#ifndef SYN_USE_RAW
	u64 handle_idx;		/**< Index to the header's handle.		*/
	#endif
	bit64 block_flags;	/**< Flag-enum bitmap of the header.		*/
} __attribute__((aligned(32))) pool_header_ext_t;

/**
 * 	Memory pool data structure.
 *
 *	@details
 *	Every pool is linked to the next and the next are linked to the previous,
 *	resulting in a linked-list style of arena. total_mem_size represents
 *	the size of all memory pools together (only in the first pool),
 *	while mem_size is the maximum size for each.
 */
typedef struct Memory_Pool {
	void *mem;			/**< Pointer to the heap region.			*/
	void *heap_base;		/**< Base of the heap, used for freeing.		*/
	struct Memory_Pool *next_pool;	/**< Pointer to the next pool.				*/
	pool_free_node_t *first_free;	/**< Pointer to the first freed header.			*/
	u32 size;			/**< Maximum allocated size for this pool in bytes.	*/
	u32 offset;			/**< How much space has been used so far in bytes.	*/
	u32 free_count;			/**< How many freed headers there are in this pool.	*/
} __attribute__((aligned(64))) memory_pool_t;

/**
 * 	Super-struct-ure for the entire arena.
 *
 *	@details Each arena is put in its own TLS so each thread has its own arena,
 *	reducing memory congestion and (hopefully) speed due to no thread locking needed,
 *	albeit at an increased complexity level as each thread manages their own heap.
 *	Every arena is nested into the first pool's heap, just before the pool structure.
 *	The heap ptr of the pool is then incremented to just past the arena and pool structs,
 *	and at the end of the allocator deadzone for the pool structure.
 *	This allows using pool->mem to point to the first header if pool->offset != 0.
 *
 * 	@details
 *	Each first pool for each arena starts at a specific size of _MAX_FIRST_POOL_SIZE_,
 *	and each pool has a maximum defined value of _MAX_POOL_SIZE_ which equates to about
 *	2 GiB. If you need more then this _per pool_, then may god help you.
 *
 * 	@details
 * 	When allocation in all pools has hit its limit (i.e, calling find_new_pool_block returns
 * 	nullptr), a new pool will be made and linked to the previous, and the previous linked to
 * 	the new, via _pool_init()_, and arena_thread->pool_count is incremented. The new pool's
 * 	size is set to the previous pool's size * 2, making further capacity limit hits twice as
 * 	unlikely compared to if it were to just do prev_pool->size + prev_pool->size.
 *
 * 	@details
 * 	Each arena also manages a singly-linked-list of handle tables, and each table is capable
 * 	of storing and logging 64 handles each for 64 total allocations per table in its own mmap'd region.
 * 	Every subsequent new handle table when the previous is full will still have the same size,
 * 	allocation and logging capacity.
 */
typedef struct Arena {
	memory_pool_t *first_mempool;	/**< Pointer to the first memory pool.		*/
	memory_pool_t *first_hp_pool;	/**< Pointer to the first huge page pool.	*/
	usize total_arena_bytes;	/**< The total size of all pools together.	*/
	#ifndef SYN_USE_RAW
	handle_table_t *first_hdl_tbl;	/**< Pointer to LL of tables, matrix.		*/
	u32 table_count;		/**< How many tables there are.			*/
	#endif
	u32 pool_count;			/**< How many memory pools there are.		*/
} __attribute__((aligned(32))) arena_t;

typedef struct Debug_VTable {
	void (*debug_print_all)();
	void (*to_console)(void (*log_stream)(const char *restrict string, va_list arg_list), const char *restrict str, ...);
	// i love this fptr, it traumatizes my programmer friends. I am NOT changing it.
} __attribute__((aligned(16))) debug_vtable_t;


// clang-format on

/*
 * It is very important to have everything aligned in memory, so we should go out of our way to make it that way.
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case. PH also stands for Pool Header.
 */

// TODO fix all usages of the new pool deadzone vs head deadzone.
static constexpr u64 POOL_DEADZONE = 0xDEADDEADDEADDEADULL;
static constexpr u32 HEAD_DEADZONE = 0xDEADDEADU;
static constexpr u16 PD_ARENA_SIZE = (sizeof(arena_t) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_POOL_SIZE = (sizeof(memory_pool_t) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_HEAD_SIZE = (sizeof(pool_header_t) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_FREE_PH_SIZE = (sizeof(pool_free_node_t) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
static constexpr u16 PD_RESERVED_F_SIZE = ((PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);

#endif //ARENA_ALLOCATOR_STRUCTS_H
