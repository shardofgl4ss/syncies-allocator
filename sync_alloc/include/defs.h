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
	#define	ALIGNMENT	8
#endif

static_assert(
	(ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= 8) && (ALIGNMENT < 128),
	"ALIGNMENT must be a power of 8 and less then 128!\n"
);

#include "structs.h"

constexpr u32 KIBIBYTE = 1024;
constexpr u32 MEBIBYTE = 1024 * KIBIBYTE;
constexpr u32 GIBIBYTE = 1024 * MEBIBYTE;

/* It is very important to have everything aligned in memory, so we should go out of our way to make it that way. *
 * PD here stands for PADDED, F for FIRST as the first arena's pool is a special case.							  */
constexpr u8 MAX_TABLE_HNDL_COLS = 64;
constexpr u16 MAX_ALLOC_SLAB_SIZE = 256;
constexpr u32 MAX_FIRST_POOL_SIZE = KIBIBYTE * 128;	// 128 KiB
constexpr u64 MAX_ALLOC_POOL_SIZE = KIBIBYTE * 64;	// 64 KiB
constexpr u64 MAX_ALLOC_HUGE_SIZE = GIBIBYTE * 4;	// 4 GiB
constexpr u32 MAX_POOL_SIZE = GIBIBYTE * 2;			// 2 GiB
constexpr u16 PD_ARENA_SIZE = (sizeof(Arena) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_POOL_SIZE = (sizeof(Memory_Pool) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_HEAD_SIZE = (sizeof(Pool_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_FREE_PH_SIZE = (sizeof(Pool_Free_Header) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_HANDLE_SIZE = (sizeof(Arena_Handle) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_TABLE_SIZE = (sizeof(Handle_Table) + (ALIGNMENT - 1)) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_RESERVED_F_SIZE = (PD_ARENA_SIZE + PD_POOL_SIZE) + (ALIGNMENT - 1) & (u16)~(ALIGNMENT - 1);
constexpr u16 PD_HDL_MATRIX_SIZE = ((PD_HANDLE_SIZE * MAX_TABLE_HNDL_COLS) + PD_TABLE_SIZE) + (ALIGNMENT - 1) & (u16)~(ALIGNMENT - 1);
constexpr u16 MINIMUM_BLOCK_ALLOC = 8 + PD_HEAD_SIZE;

#endif //ARENA_ALLOCATOR_DEFS_H