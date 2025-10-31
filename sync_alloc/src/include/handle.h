//
// Created by SyncShard on 10/15/25.
//

#ifndef ARENA_ALLOCATOR_HANDLE_H
#define ARENA_ALLOCATOR_HANDLE_H

#include "structs.h"


/// @brief Creates a new entry in the handle table wherever it fits.
/// @param head The head to link to the handle.
/// @return a ((hopefully)) valid Arena Handle. handle->addr will be NULL if it fails.
[[gnu::visibility("hidden")]]
extern struct Arena_Handle mempool_create_handle_and_entry(Pool_Header *restrict head);


/// @brief Creates a new handle table. Does not increment table_count by itself, do that before calling.
///
/// @param table The table to map.
/// @return a valid handle table if there is enough system memory.
[[gnu::visibility("hidden")]]
extern Handle_Table *mempool_new_handle_table(Handle_Table *restrict table);

#endif //ARENA_ALLOCATOR_HANDLE_H
