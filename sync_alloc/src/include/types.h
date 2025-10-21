//
// Created by SyncShard on 10/14/25.
//

#ifndef ARENA_ALLOCATOR_TYPES_H
#define ARENA_ALLOCATOR_TYPES_H

#include <sys/types.h>

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
typedef __int128_t i128;
typedef __uint128_t u128;
typedef float f32;
typedef double f64;
typedef long double f128;
typedef size_t usize;
typedef unsigned
_BitInt(32) bit32;
typedef unsigned
_BitInt(64) bit64;
typedef unsigned
_BitInt(128) bit128;
typedef unsigned
_BitInt(256) bit256;
typedef usize uintptr;
#endif //ARENA_ALLOCATOR_TYPES_H