//
// Created by SyncShard on 11/6/25.
//
// ReSharper disable CppDFAUnreachableCode
// ReSharper disable CppRedundantBooleanExpressionArgument

// ReSharper disable CppUseAuto
#include "syn_memops.h"
#include "defs.h"
#include "types.h"
#include <emmintrin.h>
#include <immintrin.h>
#include <stdint.h>


enum Simd_Width {
	WIDTH_STD = 8,
	WIDTH_SSE = 16,
	WIDTH_AVX = 32
};

enum Simd_Flags {
	SIMD_NONE = 0,
	SIMD_SSE  = 1 << 0,
	SIMD_AVX  = 1 << 1,
};

//#define __AVX2__
static constexpr int simd_capability =
	#if defined(__AVX2__) && defined(__SSE__)
		SIMD_AVX | SIMD_SSE;
	#elif defined(__AVX2__) && !defined(__SSE__)
		SIMD_AVX;
	#elif defined(__SSE__) && !defined(__AVX2__)
		SIMD_SSE;
	#else
		SIMD_NONE;
	#endif


[[gnu::hot]]
static inline void syn_memcpy_scalarcpy(u8 * restrict dest, const u8 * restrict src, const usize size)
{
	for (usize i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}


[[maybe_unused, gnu::hot]]
static inline void syn_memcpy_ssecpy(u8 * restrict dest, const u8 * restrict src, const usize size)
{
	#ifdef __SSE__
	if (((uintptr_t)src & WIDTH_SSE) == 0 && ((uintptr_t)dest & WIDTH_SSE) == 0) {
		for (usize i = 0; i + WIDTH_SSE <= size; i += WIDTH_SSE) {
			const __m128i ssereg = _mm_load_si128((const __m128i *)(src + i));
			_mm_store_si128((__m128i *)(dest + i), ssereg);
		}
	} else {
		for (usize i = 0; i + WIDTH_SSE <= size; i += WIDTH_SSE) {
			const __m128i ssereg = _mm_loadu_si128((const __m128i *)(src + i));
			_mm_storeu_si128((__m128i *)(dest + i), ssereg);
		}
	}
	#else
	syn_memcpy_scalarcpy(dest, src, size);
	#endif
}


[[maybe_unused, gnu::hot]]
static inline void syn_memcpy_avxcpy(u8 * restrict dest, const u8 * restrict src, const usize size)
{
	#ifdef __AVX2__
	if (((uintptr_t)src & WIDTH_AVX) == 0 && ((uintptr_t)dest & WIDTH_AVX) == 0) {
		for (usize i = 0; i + WIDTH_AVX <= size; i += WIDTH_AVX) {
			const __m256i avxreg = _mm256_load_si256((const __m256i *)(src + i));
			_mm256_store_si256((__m256i *)(dest + i), avxreg);
		}
	} else {
		for (usize i = 0; i + WIDTH_AVX <= size; i += WIDTH_AVX) {
			const __m256i avxreg = _mm256_loadu_si256((const __m256i *)(src + i));
			_mm256_storeu_si256((__m256i *)(dest + i), avxreg);
		}
	}
	/* We have to zero the upper bits of the AVX regs to eliminate the 100-500 cpu cycle	*
	 * transition state cost that occurs when transitioning from AVX to SSE. This only	*
	 * takes about 1-10 cycles, so it is very much worth it.				*/
	_mm256_zeroupper();
	#elifdef __SSE__
	syn_memcpy_ssecpy(dest, src, size);
	#else
	syn_memcpy_scalarcpy(dest, src, size);
	#endif
}


[[gnu::hot]]
int syn_memcpy(void *restrict destination, const void *restrict source, const usize size)
{
	if (size == 0 || !destination || !source || destination == source) {
		return 1;
	}

	u8 *dest = destination;
	const u8 *src = source;

	if (size < ALIGNMENT || simd_capability & SIMD_NONE) {
		syn_memcpy_scalarcpy(dest, src, size);
		return 0;
	}

	const uintptr_t dest_addr = (uintptr_t)dest;
	constexpr u8 simd_width = (simd_capability & SIMD_AVX)
	                          ? WIDTH_AVX
	                          : ((simd_capability & SIMD_SSE)
	                             ? WIDTH_SSE
	                             : WIDTH_STD);

	const uintptr_t misalignment = dest_addr % simd_width;
	uintptr_t head = (misalignment) ? simd_width - misalignment : 0;

	const uintptr_t body = (size - head) & ~(simd_width - 1);
	uintptr_t tail = size - head - body;
	if (head > size) {
		head = size;
	}
	if (!head) {
		goto skip_head;
	}
	if (head == WIDTH_SSE && simd_capability & SIMD_SSE) {
		syn_memcpy_ssecpy(dest, src, head);
	} else {
		syn_memcpy_scalarcpy(dest, src, head);
	}
skip_head:
	switch (simd_capability) {
		case SIMD_AVX | SIMD_SSE:
			usize remaining_bytes = size - head;
			usize avx_chunk = 0;
			if (remaining_bytes >= WIDTH_AVX) {
				avx_chunk = remaining_bytes & ~(WIDTH_AVX - 1);
				syn_memcpy_avxcpy(dest + head, src + head, avx_chunk);
				remaining_bytes -= avx_chunk;
			}
			usize sse_chunk = 0;
			if (remaining_bytes >= WIDTH_SSE) {
				sse_chunk = remaining_bytes & ~(WIDTH_SSE - 1);
				syn_memcpy_ssecpy(dest + head + avx_chunk, src + head + avx_chunk, sse_chunk);
			}
			tail = remaining_bytes - sse_chunk;
			if (tail) {
				syn_memcpy_scalarcpy(dest + head + avx_chunk + sse_chunk,
				                     src + head + avx_chunk + sse_chunk,
				                     tail);
			}
			goto skip_tail;
		case SIMD_AVX:
			syn_memcpy_avxcpy(dest + head, src + head, body);
			break;
		case SIMD_SSE:
			syn_memcpy_ssecpy(dest + head, src + head, body);
			break;
		default:
			syn_memcpy_scalarcpy(dest + head, src + head, body);
			break;
	}
	if (tail) {
		syn_memcpy_scalarcpy(dest + head + body, src + head + body, tail);
	}
skip_tail:
	return 0;
}


[[gnu::hot]]
static inline void syn_memset_scalarset(u8 * restrict target, const u8 byte, const usize size)
{
	for (int i = 0; i < size; i++) {
		target[i] = byte;
	}
}


[[maybe_unused, gnu::hot]]
static inline void syn_memset_sseset(u8 * restrict target, const u8 byte, const usize size)
{
	#ifdef __SSE__
	const __m128i ssereg = _mm_set1_epi8((char)byte);
	for (usize i = 0; i + WIDTH_SSE <= size; i += WIDTH_SSE) {
		_mm_storeu_si128((__m128i *)(target + i), ssereg);
	}
	#else
	syn_memset_scalarset(target, byte, size);
	#endif
}


[[maybe_unused, gnu::hot]]
static inline void syn_memset_avxset(u8 * restrict target, const u8 byte, const usize size)
{
	#ifdef __AVX2__
	const __m256i avxreg = _mm256_set1_epi8((char)byte);
	for (usize i = 0; i + WIDTH_AVX <= size; i += WIDTH_AVX) {
		_mm256_storeu_si256((__m256i *)(target + i), avxreg);
	}
	_mm256_zeroupper();
	#elifdef __SSE__
	syn_memset_sseset(target, byte, size);
	#else
	syn_memset_scalarset(target, byte, size);
	#endif
}


[[gnu::hot]]
int syn_memset(void *restrict target, const u8 byte, const usize bytes)
{
	if (bytes == 0 || !target) {
		return 1;
	}

	u8 *dest = target;

	if (bytes < ALIGNMENT || simd_capability & SIMD_NONE) {
		syn_memset_scalarset(dest, byte, bytes);
		return 0;
	}
	const uintptr_t dest_addr = (uintptr_t)dest;
	constexpr u8 simd_width = (simd_capability & SIMD_AVX)
	                          ? WIDTH_AVX
	                          : ((simd_capability & SIMD_SSE)
	                             ? WIDTH_SSE
	                             : WIDTH_STD);

	const uintptr_t misalignment = dest_addr % simd_width;
	uintptr_t head = (misalignment) ? simd_width - misalignment : 0;

	const uintptr_t body = (bytes - head) & ~(simd_width - 1);
	uintptr_t tail = bytes - head - body;
	if (head > bytes) {
		head = bytes;
	}
	if (head) {
		syn_memset_scalarset(dest, byte, head);
	}

	switch (simd_capability) {
		case SIMD_AVX | SIMD_SSE:
			usize remaining_bytes = bytes - head;
			usize avx_chunk = 0;
			if (remaining_bytes >= WIDTH_AVX) {
				avx_chunk = remaining_bytes & ~(WIDTH_AVX - 1);
				syn_memset_avxset(dest + head, byte, avx_chunk);
				remaining_bytes -= avx_chunk;
			}
			usize sse_chunk = 0;
			if (remaining_bytes >= WIDTH_SSE) {
				sse_chunk = remaining_bytes & ~(WIDTH_SSE - 1);
				syn_memset_sseset(dest + head + avx_chunk, byte, sse_chunk);
			}
			tail = remaining_bytes - sse_chunk;
			if (tail) {
				syn_memset_scalarset(dest + head + avx_chunk + sse_chunk,
				                     byte,
				                     tail);
			}
			goto skip_tail;
		case SIMD_AVX:
			syn_memset_avxset(dest + head, byte, body);
			break;
		case SIMD_SSE:
			syn_memset_sseset(dest + head, byte, body);
			break;
		default:
			syn_memset_scalarset(dest + head, byte, body);
			break;
	}
	if (tail) {
		syn_memset_scalarset(dest + head + body, byte, tail);
	}
skip_tail:
	return 0;
}