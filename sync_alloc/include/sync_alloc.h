//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_LIB_H
#define ARENA_ALLOCATOR_ALLOC_LIB_H

#include "defs.h"
#include "structs.h"
#include "syn_memops.h"
#include "types.h"

// clang-format off
typedef enum {
	ALLOC_SENSITIVE,
	ALLOC_ZEROED,
	ALLOC_FROZEN,
} raw_alloc_flags_t;

typedef enum {
	SYN_OK		   = 0,
	SYN_HEADER_CORRUPT = 1 << 0,
	SYN_ARENA_CORRUPT  = 1 << 1,
	SYN_HANDLE_CORRUPT = 1 << 2,
} corruption_integrity_t;
// clang-format on

/// @brief Destroys a whole arena, deallocating it.
/// @note If the arena_thread is NULL, this function does nothing.
/// @warning Each thread has its own arena.\n If multithreading, make sure to have all threads destroy their own arena.
[[gnu::visibility("default")]]
extern void syn_free_all();

#ifdef SYN_ALLOC_DISABLE_SAFETY
/// @brief Checks heap corruption in the allocator.
/// @note Corruption is only checked around the user allocation, and at the base of each pool.\n
/// This might be changed later to walk all headers and report all known corruption.
/// @param user_hdl The handle to check the integrity with.
/// @return
/// 0 if there has been no deadzone corruption.\n
/// 1 if header corruption has been detected.\n
/// 2 if arena corruption has been detected.\n
/// 3 if provided handle is corrupted or invalid.
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
 */
[[nodiscard, gnu::visibility("default")]]
extern syn_handle_t syn_alloc(usize size);


[[nodiscard, gnu::visibility("default")]]
inline static syn_handle_t syn_calloc(const usize size) {
	const syn_handle_t hdl = syn_alloc(size);
	syn_memset(hdl.addr, 0, hdl.header->size - PD_HEAD_SIZE - DEADZONE_PADDING);
	return hdl;
}


/**
 * @brief "Fake clears", or resets, all allocations. Less overhead then freeing all.
 *
 * @details This does deallocate allocations, but only excess pools and handle tables are deallocated.\n
 * The first memory pool and handle table are kept, but reset to their original no-allocations state.\n
 * All active handles should be dropped if this is called, as they will all become invalid.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[gnu::visibility("default")]]
extern void syn_reset();


/**
 * @brief Marks an allocated block via a user's handle as free,
 * then performs light defragmentation.
 * @param user_handle The handle to mark as free.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[gnu::visibility("default")]]
extern void syn_free(syn_handle_t *user_handle);


/**
 * @brief Reallocates a user's block.
 * @param user_handle The handle to the block to reallocate.
 * @param size The new size for the allocation.
 * @returns a 0 if reallocation succeeds,
 * and the user's handle is updated. If reallocation fails,
 * the users handle is _not_ updated, and a 1 is returned.
 * @note If the handle is frozen and reallocation is attempted, nothing will happen.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[gnu::visibility("default")]]
extern int syn_realloc(syn_handle_t *user_handle, usize size);


/**
 * @brief Freezes (or locks), a handle for the user to use.
 * @param user_handle The handle to lock.
 * @return void ptr to the block of user memory.
 * @note When finished, the handle should be unlocked to allow defragmentation.
 * @warning If the arena_thread is NULL, the library will terminate.
 */
[[nodiscard, gnu::visibility("default")]]
extern void *syn_freeze(syn_handle_t *user_handle);


/**
 * @brief Thaws a handle for defragmentation, or when the user is done with the allocation.\n
 * The void ptr can no longer be used.
 * @param user_handle The handle to unlock/thaw.
 * @warning If the arena_thread is NULL, or if corruption is detected, the library will terminate.
 */
[[gnu::visibility("default")]]
extern void syn_thaw(const syn_handle_t *user_handle);

#endif //ARENA_ALLOCATOR_ALLOC_LIB_H
