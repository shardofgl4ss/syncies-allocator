//
// Created by SyncShard on 11/6/25.
//

#ifndef ARENA_ALLOCATOR_SYN_MEMOPS_H
#define ARENA_ALLOCATOR_SYN_MEMOPS_H

#include "types.h"

[[gnu::hot]]
extern int syn_memcpy(void *restrict destination, const void *restrict source, usize size);

[[gnu::hot]]
extern int syn_memset(void *restrict target, u8 c, usize bytes);

#endif //ARENA_ALLOCATOR_SYN_MEMOPS_H