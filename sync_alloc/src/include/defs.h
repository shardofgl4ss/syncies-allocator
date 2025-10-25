//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#if !defined(__linux__)
#	warning warning: sync_alloc is not supported on platforms except for Linux.
#endif

#if !defined(__x86_64__)
static_assert(0, "sync_alloc requires 64 bit x86 architecture.");
#endif

#ifdef __GNUC__
#	if !defined(__clang__)
#		define ATTR_MALLOC(f, x) __attribute__((malloc(f, x)))
#	else
/*		Clang doesn't have __attribute__((malloc(a, b))) yet :( */
#		define ATTR_MALLOC(f, x)
#	endif
#	define ATTR_ALLOC_SIZE(i) __attribute__ ((alloc_size (i)))
#	define ATTR_PUBLIC __attribute__ ((visibility ("default")))
#	define ATTR_PRIVATE __attribute__ ((visibility ("hidden")))
#	define ATTR_PROTECTED __attribute__ ((visibility ("protected")))
#	define ATTR_PURE __attribute__ ((pure))
#	define ATTR_CONST __attribute__ ((const))
#	define ATTR_INLINE __attribute__ ((always_inline))
#	define ATTR_HOT __attribute__ ((hot))
#	define ATTR_COLD __attribute__ ((cold))
#	define ATTR_NOTHROW __attribute__ ((nothrow))
#else
#	define ATTR_ALLOC_SIZE(i)
#	define ATTR_PUBLIC
#	define ATTR_PRIVATE
#	define ATTR_INTERNAL
#	define ATTR_PROTECTED
#	define ATTR_PURE
#	define ATTR_CONST
#	define ATTR_INLINE
#	define ATTR_HOT
#	define ATTR_COLD
#	define ATTR_NOTHROW
#endif

/* Needs to be in multiples of 8.
 * An alignment of 16 however, is optimal for
 * SIMD instructions, while 32 is optimal for
 * AVX512 instructions. So it is left to the
 * user to decide.
 */
#ifndef ALIGNMENT
#	define	ALIGNMENT 8
#endif

static_assert(
	(ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= 8) && (ALIGNMENT < 33),
	"ALIGNMENT must be either 8, 16 or 32!\n"
);

#define ALLOC_DEBUG 1

#define KIBIBYTE 1024
#define MEBIBYTE (1024 * KIBIBYTE)
#define GIBIBYTE (1024 * MEBIBYTE)
#define MAX_ALLOC_HUGE_SIZE (GIBIBYTE * 4)		// 4 GiB
#define MAX_ALLOC_POOL_SIZE (KIBIBYTE * 128)	// 128 KiB
#define MAX_FIRST_POOL_SIZE (KIBIBYTE * 128)	// 128 KiB
#define MAX_POOL_SIZE (GIBIBYTE * 2)			// 2 GiB
#define MAX_TABLE_HNDL_COLS 64					// 64 B
#define MAX_ALLOC_SLAB_SIZE 256					// 256 B
#define MINIMUM_BLOCK_ALLOC 64

#endif //ARENA_ALLOCATOR_DEFS_H
