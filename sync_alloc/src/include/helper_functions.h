//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include <sys/mman.h>
#include "structs.h"


/// @brief Destroys a heap allocation.
/// @param mem The heap to destroy.
/// @param bytes The size of the heap.
/// @return 0 if successful, -1 for errors.
ATTR_INLINE static int
mp_helper_destroy(void *restrict mem, const usize bytes) { return munmap(mem, bytes); }


/// @brief Allocates memory via mmap(). Each map is marked NORESERVE and ANONYMOUS.
/// @param bytes How many bytes to allocate.
/// @return voidptr to the heap region.
ATTR_INLINE ATTR_MALLOC(mp_helper_destroy, 1) ATTR_ALLOC_SIZE(1)

static void *
mp_helper_map_mem(const usize bytes)
{
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}


/// @brief Pads a value by alignment value of ALIGNMENT defined elsewhere.
/// @param input The value to align by 8.
/// @return aligned-by-8 value.
ATTR_INLINE ATTR_CONST

static usize
mp_helper_add_padding(const usize input) { return (input + (ALIGNMENT - 1)) & (usize)~(ALIGNMENT - 1); }


/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
ATTR_INLINE ATTR_PURE

static void *
mp_helper_return_block_addr(Pool_Header *head) { return (char *)head + PD_HEAD_SIZE; }


/// @brief Calculates a handle's row via division.
/// @param hdl The handle to get the row from.
/// @return the handle index's row.
ATTR_INLINE ATTR_PURE

static u32
mp_helper_return_matrix_row(const Arena_Handle *restrict hdl) { return hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS; }


/// @brief Calculates a handle's column via modulo.
/// @param hdl The handle to get the column from.
/// @return the handle index's column.
ATTR_INLINE ATTR_PURE

static u32
mp_helper_return_matrix_col(const Arena_Handle *restrict hdl) { return hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS; }


// everything that is in the .c
static Arena *
mp_helper_return_base_arena(const Arena_Handle *restrict user_handle);
static bool
mp_helper_handle_generation_checksum(const Arena *restrict arena, const Arena_Handle *restrict hdl);
static void
mp_helper_update_table_generation(const Arena_Handle *restrict hdl);

#endif //ARENA_ALLOCATOR_HELPER_FUNCTIONS_H