//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include "defs.h"
#include "structs.h"

__attribute__ ((__always_inline__))
inline static void *
mp_helper_map_mem(const usize bytes)
{
	return mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

__attribute__ ((__always_inline__, const))
inline static usize
mp_helper_add_padding(const usize input) { return (input + (ALIGNMENT - 1)) & (usize)~(ALIGNMENT - 1); }

/// @brief Destroys a heap allocation.
/// @param mem The heap to destroy.
/// @param bytes The size of the heap.
/// @return 0 if successful, -1 for errors.
__attribute__ ((__always_inline__))
inline static int
mp_helper_destroy(void *restrict mem, const usize bytes) { return munmap(mem, bytes); }

/// @brief Returns the voidptr of a valid header's block.
/// @param head Header ptr to the header to do ptr arithmetic,
/// to calculate the location of the block.
__attribute__((__always_inline__, __pure__))
inline static void *
mp_helper_return_block_addr(Pool_Header *head) { return (char *)head + PD_HEAD_SIZE; }

__attribute__((__always_inline__, pure))
inline static u32
mp_helper_return_matrix_row(const Arena_Handle *restrict hdl)
{
	return hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS;
}

__attribute__((__always_inline__, pure))
inline static u32
mp_helper_return_matrix_col(const Arena_Handle *restrict hdl)
{
	return hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS;
}

// everything that is in the .c
static Arena *
mp_helper_return_base_arena(const Arena_Handle *restrict user_handle);
static bool
mp_helper_handle_generation_checksum(const Arena *restrict arena, const Arena_Handle *restrict hdl);
static void
mp_helper_update_table_generation(const Arena_Handle *restrict hdl);

#endif //ARENA_ALLOCATOR_HELPER_FUNCTIONS_H