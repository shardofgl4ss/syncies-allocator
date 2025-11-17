//
// Created by SyncShard on 11/17/25.
//

#ifndef ARENA_ALLOCATOR_DEADZONE_H
#define ARENA_ALLOCATOR_DEADZONE_H

#include "structs.h"
#include "types.h"

/**
 * Checks for corruption in a given header.
 *
 * @param head The header to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
extern bool corrupt_header_check(pool_header_t *restrict head);

/**
 * Checks for corruption in a given pool.
 *
 * @param pool The pool to check for corruption.
 * @return False if no corruption is found, true otherwise.
 */
extern bool corrupt_pool_check(memory_pool_t *pool);

extern u32 return_prev_block_size(pool_header_t *head);
extern int create_head_deadzone(const pool_header_t *head, memory_pool_t *pool);
extern int create_pool_deadzone(memory_pool_t *pool);


#endif //ARENA_ALLOCATOR_DEADZONE_H