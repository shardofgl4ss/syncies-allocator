//
// Created by SyncShard on 10/26/25.
//

#ifndef ARENA_ALLOCATOR_ALLOC_UTILS_H
#define ARENA_ALLOCATOR_ALLOC_UTILS_H

#include "structs.h"
/// @brief Handles the sentinel flags of the current and surrounding headers.
/// @param head The header to index.
ATTR_PRIVATE extern void
update_sentinel_flags(Pool_Header *head);
/// @brief Handle generation checksum.
/// @param hdl the handle to checksum.
/// @return returns true if checksum with the handle and entry is true, and false if not.
ATTR_PRIVATE extern bool
handle_generation_checksum(const struct Arena_Handle *restrict hdl);
/// @brief Updated the entry's generation, usually after a free.
/// @param hdl The handle to update the table generation.
ATTR_PRIVATE extern void
update_table_generation(const struct Arena_Handle *restrict hdl);

ATTR_PRIVATE extern void
table_destructor();

ATTR_PRIVATE extern void
pool_destructor();

#endif //ARENA_ALLOCATOR_ALLOC_UTILS_H
