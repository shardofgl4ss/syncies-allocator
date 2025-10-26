//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_LIB_H
#define ARENA_ALLOCATOR_ALLOC_LIB_H

#include "../src/include/structs.h"


///	@brief Creates a new arena. Each arena is thread-local and doubles in size with each new pool.
///	@return 0 on success, 1 for failure.
ATTR_PUBLIC extern int
arena_create();


/// @brief Destroys a whole arena, deallocating it and setting all values to NULL or 0
/// @warning Each thread has its own arena. If multithreading, make sure to have all threads destroy their own.
ATTR_PUBLIC extern void
arena_destroy();


/**	@brief Clears up defragmentation of the memory pool where there is any.
 *
 *	@details Indexes through the memory pool from first to last,
 *	if there are two adjacent blocks that are free, links the first free
 *	to the next non-free, then updates head->chunk_size of the first free.
 *
 *	@param light_flag The light defrag option. If true, only a light defragmentation
 *	will occur. If false, heavy defragmentation will occur.
 */
ATTR_PUBLIC extern void
arena_defragment(bool light_flag);


/** @brief Allocates a new block of memory.
 *
 *	@param size How many bytes the user requests.
 *	@return arena handle to the user, use instead of a vptr.
 *
 *	@note All size is rounded up to the nearest value of ALIGNMENT, and a minimum valid size is 8 bytes.
 *	@warning If the arena is NULL at this point, the library will terminate.
 */
[[nodiscard]] ATTR_PUBLIC extern struct Arena_Handle
arena_alloc(usize size);


/**
 * @brief Clears all pools, deleting all but the first pool, performing a full reset, unless 1 is given.
 * @param reset_type 0: Will full reset the entire arena.
 * 1: Will soft reset the arena, not deallocating excess pools.
 * 2: Will do the same as 0, but will not wipe the first arena.
 */
ATTR_PUBLIC extern void
arena_reset(int reset_type);


/**	@brief Marks an allocated block via a user's handle as free,
 *	then performs light defragmentation.
 *
 *	@param user_handle The handle to mark as free.
 */
ATTR_PUBLIC extern void
arena_free(struct Arena_Handle *user_handle);


/** @brief Reallocates a user's block.
 *	@param user_handle The handle to the block to reallocate.
 *	@param size The new size for the allocation.
 *	@returns a 0 if reallocation succeeds,
 *	and the user's handle is updated. If reallocation fails,
 *	the users handle is _not_ updated, and a 1 is returned.
 *
 *	@note If the handle is frozen and reallocation is attempted, nothing will happen.
 */
ATTR_PUBLIC extern int
arena_realloc(struct Arena_Handle *user_handle, usize size);


/** @brief Locks a handle for the user to use.
 *	@param user_handle The handle to lock.
 *	@return vptr to the block of user memory.
 *	@note When finished, the handle should be unlocked to allow defragmentation.
 */
[[nodiscard]] ATTR_PUBLIC extern void *
handle_lock(struct Arena_Handle *user_handle);


/**	@brief Unlocks a handle for defragmentation. The vptr can no longer be used.
 *	@param user_handle
 */
ATTR_PUBLIC extern void
handle_unlock(const struct Arena_Handle *user_handle);

#endif //ARENA_ALLOCATOR_ALLOC_LIB_H
