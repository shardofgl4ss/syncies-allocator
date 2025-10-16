//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#if !defined(__linux__)
#	warning warning: sync_allocator is not supported on platforms except for Linux.
#endif

#if !defined(__x86_64__)
static_assert(0, "sync_allocator requires 64 bit x86 architecture.");
#endif

#ifdef __GNUC__
#	if __GNUC__ >= 11 && !defined(__clang__)
#		define ATTR_MALLOC(f, x) __attribute__((malloc(f, x)))
#	else
/*		Clang doesn't have __attribute__((malloc(a, b))) yet :( */
#		define ATTR_MALLOC(f, x)
#	endif
#	define ATTR_ALLOC_SIZE(i) __attribute__((alloc_size(i)))
#	define ATTR_PUBLIC __attribute__((visibility("default")))
#	define ATTR_PRIVATE __attribute__((visibility("hidden")))
#	define ATTR_PURE __attribute__((pure))
#	define ATTR_CONST __attribute__((const))
#	define ATTR_INLINE __always_inline
#else
#	define ATTR_ALLOC_SIZE(i)
#	define ATTR_PUBLIC
#	define ATTR_PRIVATE
#	define ATTR_PURE
#	define ATTR_CONST
#	define ATTR_INLINE
#endif

/* Needs to be in multiples of 8.
 * An alignment of 16 however, is optimal for
 * SIMD instructions, while 32 is optimal for
 * AVX512 instructions. So it is left to the
 * user to decide.
 */
#ifndef ALIGNMENT
#	define	ALIGNMENT	8
#endif
static_assert(
	(ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= 8) && (ALIGNMENT < 128),
	"ALIGNMENT must be a power of 8 and less then 128!\n"
);

#ifndef ALLOC_DEBUG_LVL
#	define ALLOC_DEBUG_LVL 2
#endif
static_assert(
	ALLOC_DEBUG_LVL >= 0 && ALLOC_DEBUG_LVL <= 2,
	"ALLOC_DEBUG_LVL must be either 0, 1, or 2!\n"
);

#define KIBIBYTE 1024
#define MEBIBYTE (1024 * KIBIBYTE)
#define GIBIBYTE (1024 * MEBIBYTE)
#define MAX_ALLOC_HUGE_SIZE (GIBIBYTE * 4)		// 4 GiB
#define MAX_ALLOC_POOL_SIZE (KIBIBYTE * 64)		// 64 KiB
#define MAX_FIRST_POOL_SIZE (KIBIBYTE * 128)	// 128 KiB
#define MAX_POOL_SIZE (GIBIBYTE * 2)			// 2 GiB
#define MAX_TABLE_HNDL_COLS 64					// 64 B
#define MAX_ALLOC_SLAB_SIZE 256					// 256 B
#define MINIMUM_BLOCK_ALLOC ALIGNMENT

#endif //ARENA_ALLOCATOR_DEFS_H