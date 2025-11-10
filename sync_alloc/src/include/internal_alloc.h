//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "structs.h"
#include "types.h"
#include <stdint.h>

// wow this header sure is empty

/**
 * Analyzes the header-block chain, making a new header if there is no free one to use.
 * If there is no space at all in the pool, it will create a new pool for it.
 *
 * @param requested_size User-requested size, aligned by ALIGNMENT.
 * @return ptr to header of a valid block in the heap, if creating one was successful.
 *
 * @warning Will return NULL if a new pool's header could not be made, which should be impossible.
 * It will only happen if all pools cannot fit the allocation AND mmap fails.
 */
[[gnu::visibility("hidden")]]
extern pool_header_t *find_or_create_new_header(u32 requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H