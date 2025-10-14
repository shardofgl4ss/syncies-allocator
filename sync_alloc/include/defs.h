//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#ifndef __linux__
#pragma message("sync_allocator is not supported on platforms except for Linux.")
#endif

#ifndef __x86_64__
static_assert(0, "sync_allocator requires 64 bit x86 architecture.");
#endif

/* Needs to be in multiples of 8.
 * An alignment of 16 however, is optimal for
 * SIMD instructions, while 32 is optimal for
 * AVX512 instructions. So it is left to the
 * user to decide.
 */
#ifndef ALIGNMENT
	#define	ALIGNMENT				8
#endif

static_assert(
	(ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= 8) && (ALIGNMENT < 32768),
	"ALIGNMENT must be a power of 8 and less then 32768!\n"
);

#define MAX_TABLE_HNDL_COLS			64
#define MAX_ALLOC_SLAB_SIZE			128
#define MAX_FIRST_POOL_SIZE			16384
#define MAX_ALLOC_POOL_SIZE			65536
#define MAX_ALLOC_HUGE_SIZE			4294967296

#include "structs.h"

/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way. *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case.							  */
constexpr u_int16_t PD_ARENA_SIZE = (sizeof(Arena) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_POOL_SIZE = (sizeof(Memory_Pool) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_HEAD_SIZE = (sizeof(Pool_Header) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_FREE_PH_SIZE = (sizeof(Pool_Free_Header) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_PFH_DATA_SIZE = (sizeof(Pool_Free_Header_Data) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_HANDLE_SIZE = (sizeof(Arena_Handle) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_TABLE_SIZE = (sizeof(Handle_Table) + (ALIGNMENT - 1)) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_RESERVED_F_SIZE = (PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t PD_HDL_MATRIX_SIZE = ((PD_HANDLE_SIZE * MAX_TABLE_HNDL_COLS) + PD_TABLE_SIZE) + (ALIGNMENT - 1) & (u_int16_t)~(ALIGNMENT - 1);
constexpr u_int16_t MINIMUM_BLOCK_ALLOC = ALIGNMENT + 8 + PD_HEAD_SIZE;

#endif //ARENA_ALLOCATOR_DEFS_H