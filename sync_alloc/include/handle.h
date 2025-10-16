//
// Created by SyncShard on 10/15/25.
//

#ifndef ARENA_ALLOCATOR_HANDLE_H
#define ARENA_ALLOCATOR_HANDLE_H

#include "helper_functions.h"

static Arena_Handle
mempool_create_handle_and_entry(Arena *restrict arena, Pool_Header *restrict head);

/** @brief Creates a new handle table. Does not increment table_count by itself, do that before calling.
 *
 * @param arena The arena to create a new table on.
 * @param table The table to map.
 * @return a valid handle table if there is enough system memory.
 */
static Handle_Table *
mempool_new_handle_table(Arena *restrict arena, Handle_Table *restrict table);

#endif //ARENA_ALLOCATOR_HANDLE_H