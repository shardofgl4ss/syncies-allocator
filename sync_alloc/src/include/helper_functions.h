//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include "defs.h"
#include "structs.h"
#include "types.h"
#include <stdint.h>

#include <sys/mman.h>


/// @brief Destroys a heap allocation.
/// @param mem The heap to destroy.
/// @param bytes The size of the heap.
/// @return 0 if successful, -1 for errors.
[[maybe_unused]]
static inline int helper_destroy(void *restrict mem, const usize bytes)
{
	return munmap(mem, bytes);
}


/// @brief Allocates memory via mmap(). Each map is marked NORESERVE and ANONYMOUS.
/// @param bytes How many bytes to allocate.
/// @return voidptr to the heap region.
[[maybe_unused, nodiscard, gnu::malloc(helper_destroy, 1), gnu::alloc_size(1)]]
static inline void *helper_map_mem(const usize bytes)
{
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}


/// @brief Use the ADD_PADDING() or ADD_ALIGNMENT_PADDING() define.
[[maybe_unused, gnu::const, deprecated]]
static inline usize helper_add_padding(const usize input)
{
	return (input + (ALIGNMENT - 1)) & (usize)~(ALIGNMENT - 1);
}


/// @brief Use the BLOCK_ALIGN_PTR() define.
[[maybe_unused, deprecated]]
static inline void *helper_return_block_addr(pool_header_t *head)
{
	return (void *)((((uintptr_t)((char *)head + PD_HEAD_SIZE)) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1));
}


/// @brief Calculates a handle's row via division.
/// @param hdl The handle to get the row from.
/// @return the handle index's row.
[[maybe_unused, gnu::pure]]
static inline u32 helper_return_matrix_row(const struct Syn_Handle *restrict hdl)
{
	return hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS;
}


/// @brief Calculates a handle's column via modulo.
/// @param hdl The handle to get the column from.
/// @return the handle index's column.
[[maybe_unused, gnu::pure]]
static inline u32 helper_return_matrix_col(const struct Syn_Handle *restrict hdl)
{
	return hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS;
}

#endif // ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
