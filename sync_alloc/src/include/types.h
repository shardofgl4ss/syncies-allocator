//
// Created by SyncShard on 10/14/25.
//

#ifndef ARENA_ALLOCATOR_TYPES_H
#define ARENA_ALLOCATOR_TYPES_H

#include <stddef.h>
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
typedef ptrdiff_t intptr;
typedef _BitInt(8) bit8;
typedef _BitInt(16) bit16;
typedef _BitInt(32) bit32;
typedef _BitInt(64) bit64;
typedef _BitInt(128) bit128;
typedef _BitInt(256) bit256;
typedef _BitInt(512) bit512;
typedefx _BitInt(1024) bit1024;

#endif //ARENA_ALLOCATOR_TYPES_H