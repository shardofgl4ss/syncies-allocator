//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#ifndef __linux__
#pragma message("sync_allocator is not supported on platforms except for Linux.")
#endif

#if __SIZEOF_POINTER__ != 8
static_assert(0, "Non-64 bit architectures are not supported.");
#endif

/* Needs to be in multiples of 8.
 * An alignment of 16 however, is optimal for
 * SIMD instructions, while 32 is optimal for
 * AVX512 instructions. So it is left to the
 * user to decide.
 */
#ifndef ALIGNMENT
	#define	ALIGNMENT	8
#endif

static_assert(
	(ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= 8),
	"ALIGNMENT must be a power of 2 and a multiple of 8"
);

#define FIRST_POOL_ALLOC	16384
#define MINIMUM_POOL_BLOCK_ALLOC	8
#define TABLE_MAX_COL	64

#include "structs.h"

/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way. *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case.							  */
constexpr size_t PD_ARENA_SIZE = (sizeof(Arena) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_POOL_SIZE = (sizeof(Memory_Pool) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_HEAD_SIZE = (sizeof(Pool_Header) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_HANDLE_SIZE = (sizeof(Arena_Handle) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_TABLE_SIZE = (sizeof(Handle_Table) + (ALIGNMENT - 1)) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_RESERVED_F_SIZE = (PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1) & (size_t)~(ALIGNMENT - 1);
constexpr size_t PD_HDL_MATRIX_SIZE = ((PD_HANDLE_SIZE * TABLE_MAX_COL) + PD_TABLE_SIZE) + (ALIGNMENT - 1) & (size_t)~(ALIGNMENT - 1);


#endif //ARENA_ALLOCATOR_DEFS_H