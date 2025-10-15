//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_LIB_H
#define ARENA_ALLOCATOR_ALLOC_LIB_H


#include "helper_functions.h"
#include "structs.h"

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
PUBLIC_ALLOC_API Arena *
arena_create();

/// @brief Destroys a whole arena, deallocating it and setting all values to NULL or 0
/// @param arena The arena to destroy
PUBLIC_ALLOC_API void
arena_destroy(const Arena *restrict arena);

/**	@brief Clears up defragmentation of the memory pool where there is any.
 *
 *	@details Indexes through the memory pool from first to last,
 *	if there are two adjacent blocks that are free, links the first free
 *	to the next non-free, then updates head->chunk_size of the first free.
 *
 *	@param arena Arena to defragment.
 *	@param l_defrag The light defrag option. If true, only a light defragmentation
 *	will occur. If false, heavy defragmentation will occur.
 */
PUBLIC_ALLOC_API void
arena_defragment(const Arena *arena, bool l_defrag);

/** @brief Allocates a new block of memory.
 *
 *	@param arena Pointer to the arena to work on.
 *	@param size How many bytes the user requests.
 *	@return arena handle to the user, use instead of a vptr.
 *
 *	@note All size is rounded up to the nearest value of ALIGNMENT, and a minimum valid size is 8 bytes.
 *	@warning If a size of zero is provided or a sub-function fails, NULL is returned.
 */
PUBLIC_ALLOC_API Arena_Handle
arena_alloc(Arena *arena, size_t size);

/**
 * @brief Clears all pools, deleting all but the first pool, performing a full reset, unless 1 is given.
 * @param arena The arena to reset.
 * @param reset_type 0: Will full reset the entire arena.
 * 1: Will soft reset the arena, not deallocating excess pools.
 * 2: Will do the same as 0, but will not wipe the first arena.
 */
PUBLIC_ALLOC_API void
arena_reset(const Arena *restrict arena, int reset_type);

/**	@brief Marks an allocated block via a user's handle as free,
 *	then performs light defragmentation.
 *
 *	@param user_handle The handle to mark as free.
 */
PUBLIC_ALLOC_API void
arena_free(Arena_Handle *user_handle);

/** @brief Reallocates a user's block.
 *	@param user_handle The handle to the block to reallocate.
 *	@param size The new size for the allocation.
 *	@returns a 0 if reallocation succeeds,
 *	and the user's handle is updated. If reallocation fails,
 *	the users handle is _not_ updated, and a 1 is returned.
 *
 *	@note If the handle is frozen and reallocation is attempted, nothing will happen.
 */
PUBLIC_ALLOC_API int
arena_realloc(Arena_Handle *user_handle, size_t size);

/** @brief Locks a handle for the user to use.
 *	@param user_handle The handle to lock.
 *	@return vptr to the block of user memory.
 *	@note When finished, the handle should be unlocked to allow defragmentation.
 */
PUBLIC_ALLOC_API void *
handle_lock(Arena_Handle *user_handle);

/**	@brief Unlocks a handle for defragmentation. The vptr can no longer be used.
 *	@param user_handle
 */
PUBLIC_ALLOC_API void
handle_unlock(Arena_Handle *user_handle);

/** @brief Prints all memory usage and debug information of the entire arena.
 *	@param arena The arena to print debug information on.
 */
PUBLIC_ALLOC_API void
arena_debug_print_memory_usage(const Arena *arena);

#endif //ARENA_ALLOCATOR_ALLOC_LIB_H