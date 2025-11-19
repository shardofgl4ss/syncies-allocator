//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_DEFS_H
#define ARENA_ALLOCATOR_DEFS_H

#ifndef __linux__
	#warning warning: sync_alloc is not supported on platforms except for Linux. This may not compile without changes.
#endif

//#ifndef __x86_64__
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
 *
 * This might be frozen at an alignemnt of 16/32/64 for raw ptr compatibility.
 */
#define	ALIGNMENT 16

#define SYN_ALLOC_HANDLE 1

#ifndef SYN_ALLOC_HANDLE
	#define SYN_USE_RAW 1
#endif

#define PADDING 8
#define MIN_ALIGN 16
#define MAX_ALIGN 64

static_assert(((ALIGNMENT & (ALIGNMENT - 1)) == 0 && (ALIGNMENT >= MIN_ALIGN) && (ALIGNMENT <= MAX_ALIGN)) != 0,
	      "ALIGNMENT must be either 16, 32 or 64!\n");

#define ADD_PADDING(x)                      \
	(((x) + (PADDING - 1)) & (typeof(x))~(PADDING - 1))
#define ADD_ALIGNMENT_PADDING(x)                     \
	(((x) + (ALIGNMENT - 1)) & (typeof(x))~(ALIGNMENT - 1))
#define BLOCK_ALIGN_PTR(head, align)                 \
	((((uintptr_t)(head) + PD_HEAD_SIZE) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_PTR(ptr, align) \
	((((uintptr_t)(ptr)) + ((align) - 1)) & ~((align) - 1))
#define IS_ALIGNED(ptr, align)                 \
	(((char *)(ptr) & ((align) - 1)) == 0)

//#define ALLOC_DEBUG 1


#endif //ARENA_ALLOCATOR_DEFS_H

