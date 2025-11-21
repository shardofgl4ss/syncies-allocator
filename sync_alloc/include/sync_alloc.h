//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_LIB_H
#define ARENA_ALLOCATOR_ALLOC_LIB_H

#include "defs.h"
#include "structs.h"
#include <stddef.h>
#include <sys/types.h>

#ifdef SYN_ALLOC_HANDLE
/**
 * 	Arena structure for use by the user.
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
typedef struct Syn_Handle {
	void *addr;   /**< address to the user's block. Equivalent to *header + sizeof(header). */
	pool_header_t *header;  /**< pointer to the sentinel header of the user's block.		  */
	u_int32_t
		generation; /**< generation of pointer, to detect stale handles and use-after-frees.  */
	//u_int32_t handle_matrix_index;	/**< flattened matrix index.						  */
} __attribute__((aligned(32))) syn_handle_t;
#endif


// clang-format off
typedef enum {
	ALLOC_SENSITIVE,
	ALLOC_ZEROED,
	ALLOC_FROZEN,
} raw_alloc_flags_t;

typedef enum {
	SYN_OK		   = 0,
	SYN_HEADER_CORRUPT = (1 << 0),
	SYN_ARENA_CORRUPT  = (1 << 1),
	SYN_HANDLE_CORRUPT = (1 << 2),
} corruption_integrity_t;
// clang-format on

/**
 * @brief Destroys a whole arena, deallocating it.
 * @note If the arena_thread is NULL, this function does nothing.
 * @warning Each thread has its own arena.\n If multithreading, make sure to have all threads destroy their own arena.
 */
[[gnu::visibility("default")]]
extern void syn_destroy();

/**
 * @brief "Fake clears", or resets, all allocations. Less overhead then destroying the arena.
 *
 * @details This does deallocate allocations, but only excess pools and handle tables are deallocated.\n
 * The first memory pool and handle table are kept, but reset to their original no-allocations state.\n
 * All active handles should be dropped if this is called, as they will all become invalid.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[gnu::visibility("default")]]
extern void syn_reset();

#ifndef SYN_USE_RAW
#ifdef SYN_ALLOC_DISABLE_SAFETY

/**
 * @brief Checks heap corruption in the allocator.
 * @note Corruption is only checked around the user allocation, and at the base of each pool.\n
 * This might be changed later to walk all headers and report all known corruption.
 * @param user_hdl The handle to check the integrity with.
 * @return
 * 0 if there has been no deadzone corruption.\n
 * 1 if header corruption has been detected.\n
 * 2 if arena corruption has been detected.\n
 * 3 if provided handle is corrupted or invalid.
 */
[[gnu::visibility("default")]]
extern int syn_check_integrity(struct Arena_Handle *user_hdl);

#endif

/**
 * @brief Allocates a new block of memory.
 *
 * @param size How many bytes the user requests.
 * @return arena handle to the user, use instead of a vptr.
 *
 * @note All size is rounded up to the nearest value of ALIGNMENT, and a minimum valid size is 8 bytes.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 *
 * @warning Long term use of pools will build up heap junk data, if you need a zeroed allocation, use syn_calloc.
 */
[[nodiscard, gnu::visibility("default")]]
extern syn_handle_t syn_alloc(size_t size);

/**
 * @brief Allocates a new block of memory, guaranteed to be zeroed.
 *
 * @param size How many bytes to allocate to the heap.
 * @return arena handle to the user, instead of a voidptr.
 */
[[nodiscard, gnu::visibility("default")]]
extern syn_handle_t syn_calloc(size_t size);

/**
 * @brief Marks an allocated block as free, then performs defragmentation.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */

[[gnu::visibility("default")]]
extern void syn_free(syn_handle_t *user_handle);


/**
 * @brief Reallocates a user's block.
 * @param size The new size for the allocation.
 * @returns a 0 if reallocation succeeds, 1 for failure.
 * @note If the handle is frozen and reallocation is attempted, nothing will happen.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[nodiscard, gnu::visibility("default")]]
extern int syn_realloc(syn_handle_t *, size_t size);


/**
 * @brief Freezes (or locks), an allocation for the user to use.
 * @return void ptr to the block of user memory.
 * @note When finished, the handle should be unlocked to allow defragmentation.
 * @warning If the arena_thread is NULL, the library will terminate.
 */

[[nodiscard, gnu::visibility("default")]]
extern void *syn_freeze(syn_handle_t *);


/**
 * @brief Thaws an allocation, or when the user is done with the allocation.\n
 * The void ptr can no longer be used.
 *
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 * @warning The user is required to update their handle via this function when thawing,
 * if they are to pass the original handle to other allocator functions.
 */
[[nodiscard, gnu::visibility("default")]]
extern syn_handle_t syn_thaw(void *block_ptr);

#else

[[nodiscard, gnu::visibility("default")]]
extern void *syn_alloc(usize size);

[[nodiscard, gnu::visibility("default")]]
extern void *syn_calloc(usize size);

[[gnu::visibility("default")]]
extern void syn_free(void *);

[[gnu::visibility("default")]]
extern int syn_realloc(void *, usize size);

[[nodiscard, gnu::visibility("default")]]
extern void *syn_freeze(void *);

[[gnu::visibility("default")]]
extern void syn_thaw(void *);

#endif

#endif //ARENA_ALLOCATOR_ALLOC_LIB_H
