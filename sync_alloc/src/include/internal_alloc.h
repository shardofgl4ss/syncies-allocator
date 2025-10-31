//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_INTERNAL_ALLOC_H
#define ARENA_ALLOCATOR_INTERNAL_ALLOC_H

#include "structs.h"

[[gnu::visibility("hidden")]]
extern _Thread_local Arena *arena_thread;

[[gnu::visibility("hidden")]]
typedef struct header_ctx header_ctx;

/**
 * Creates a new block header
 *
 * @param ctx Internal context struct. Avoids having to pass around multiple variables multiple times.
 * @param offset How many bytes from the base of the pool.
 * @return a valid block header pointer.
 *
 * @warning Updates offset in the pool if successful.
 * @note The header is set as ALLOCATED by default.
 */
[[gnu::visibility("hidden")]]
extern Pool_Header *mempool_create_header(const header_ctx *restrict ctx, intptr offset);


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
extern Pool_Header *find_new_pool_block(u32 requested_size);

#endif //ARENA_ALLOCATOR_INTERNAL_ALLOC_H
