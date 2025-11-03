//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include "structs.h"
#include <sys/mman.h>

/// @brief Destroys a heap allocation.
/// @param mem The heap to destroy.
/// @param bytes The size of the heap.
/// @return 0 if successful, -1 for errors.
[[maybe_unused]]
inline static int helper_destroy(void *restrict mem, const usize bytes) {
	return munmap(mem, bytes);
}

/// @brief Allocates memory via mmap(). Each map is marked NORESERVE and ANONYMOUS.
/// @param bytes How many bytes to allocate.
/// @return voidptr to the heap region.
[[maybe_unused, nodiscard, gnu::malloc(helper_destroy, 1), gnu::alloc_size(1)]]
inline static void *helper_map_mem(const usize bytes) {
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

/// @brief Pads a value by alignment value of ALIGNMENT defined elsewhere.
/// @param input The value to align by 8.
/// @return aligned-by-ALIGNMENT value.
[[maybe_unused, gnu::const]]
inline static usize helper_add_padding(const usize input) {
	return (input + (ALIGNMENT - 1)) & (usize) ~(ALIGNMENT - 1);
}

/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
[[maybe_unused, gnu::const]]
inline static void *helper_return_block_addr(pool_header_t *head) {
	return (char *)head + PD_HEAD_SIZE;
}

/// @brief Calculates a handle's row via division.
/// @param hdl The handle to get the row from.
/// @return the handle index's row.
[[maybe_unused, gnu::pure]]
inline static u32 helper_return_matrix_row(const struct Arena_Handle *restrict hdl) {
	return hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS;
}

/// @brief Calculates a handle's column via modulo.
/// @param hdl The handle to get the column from.
/// @return the handle index's column.
[[maybe_unused, gnu::pure]]
inline static u32 helper_return_matrix_col(const struct Arena_Handle *restrict hdl) {
	return hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS;
}

#endif // ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
