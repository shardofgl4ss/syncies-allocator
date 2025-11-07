//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#if !defined(__linux__)
	#warning warning: sync_alloc is not supported on platforms except for Linux.
#endif

//#if !defined(__x86_64__)
//	static_assert(0, "sync_alloc requires 64 bit x86 architecture.");
//#endif

/* moving away from normal gnu attrs because I literally cannot figure out			*
 * where and when to place normal gnu attrs. Will use C23 double-bracket attrs from now on,	*
 * but this will be kept in case I forgot to swap a normal gnu attr somewhere.			*
 * I might swap back to normal gnu attrs if I do learn, due to portability with defines.	*/

#ifdef __GNUC__
	#ifndef __clang__
		#define ATTR_MALLOC(f, x) __attribute__((malloc(f, x)))
	#else
		// Clang doesn't have __attribute__((malloc(a, b))) yet :(
		#define ATTR_MALLOC(f, x)
	#endif
	#define ATTR_ALLOC_SIZE(i) __attribute__ ((alloc_size (i)))
	#define ATTR_PUBLIC __attribute__ ((visibility ("default")))
	#define ATTR_PRIVATE __attribute__ ((visibility ("hidden")))
	#define ATTR_PROTECTED __attribute__ ((visibility ("protected")))
	#define ATTR_PURE __attribute__ ((pure))
	#define ATTR_CONST __attribute__ ((const))
	#define ATTR_INLINE __attribute__ ((always_inline))
	#define ATTR_HOT __attribute__ ((hot))
	#define ATTR_COLD __attribute__ ((cold))
	#define ATTR_NOTHROW __attribute__ ((nothrow))
#else
	#define ATTR_ALLOC_SIZE(i)
	#define ATTR_PUBLIC
	#define ATTR_PRIVATE
	#define ATTR_INTERNAL
	#define ATTR_PROTECTED
	#define ATTR_PURE
	#define ATTR_CONST
	#define ATTR_INLINE
	#define ATTR_HOT
	#define ATTR_COLD
	#define ATTR_NOTHROW
#endif

/*
 * If its not in multiples of 8, problem big yes very.
 *
 * 8-byte alignment is optimal for normal instructions.
 * 16-byte alignment is optimal for SSE instructions.
 * 32-byte alignment is optimal for AVX instructions.
 * 64-byte alignment is optimal for AVX512 instructions.
 */
// TODO actually make alignment align instead of add padding.
#ifndef ALIGNMENT
	#define	ALIGNMENT 8
#endif

#define PADDING 8
#define MIN_ALIGN 8
#define MAX_ALIGN 64

static_assert(((ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= MIN_ALIGN) && (ALIGNMENT <= MAX_ALIGN)) != 0,
	      "ALIGNMENT must be either 8, 16, 32 or 64!\n");

#include <stdint.h>

#define ADD_PADDING(x) \
	(((x) + (PADDING - 1)) & (u64) ~(PADDING - 1))
#define ADD_ALIGNMENT_PADDING(x) \
	(((x) + (ALIGNMENT - 1)) & (u64) ~(ALIGNMENT - 1))
#define ALIGN_PTR(ptr) \
	((void *)((uintptr_t)(ptr) + ((ALIGNMENT - 1)) & ~(uintptr_t)((ALIGNMENT) - 1)))
#define IS_ALIGNED(ptr, align) \
	(((uintptr_t)(ptr) & ((align) - 1)) == 0)


#define ALLOC_DEBUG 1

#define DEADZONE_PADDING sizeof(u64)

#define KIBIBYTE 1024
#define MEBIBYTE (1024 * KIBIBYTE)
#define GIBIBYTE (1024 * MEBIBYTE)
#define MINIMUM_BLOCK_ALLOC 64			// 64 B
#define MAX_ALLOC_HUGE_SIZE (GIBIBYTE * 4)	// 4 GiB
#define MAX_ALLOC_POOL_SIZE (KIBIBYTE * 128)	// 128 KiB
#define MAX_FIRST_POOL_SIZE (KIBIBYTE * 128)	// 128 KiB
#define MAX_POOL_SIZE (GIBIBYTE * 2)		// 2 GiB
#define MAX_TABLE_HNDL_COLS 64			// 64 B
#define MAX_ALLOC_SLAB_SIZE 256			// 256 B

#endif //ARENA_ALLOCATOR_DEFS_H
