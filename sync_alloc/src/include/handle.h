//
// Created by SyncShard on 10/15/25.
//

#ifndef ARENA_ALLOCATOR_HANDLE_H
#define ARENA_ALLOCATOR_HANDLE_H

#include "structs.h"
#include <stdint.h>

/// @brief Creates a new handle table. Does not increment table_count by itself, do that before calling.
///
/// @param table The table to map.
/// @return a valid handle table if there is enough system memory.
[[gnu::visibility("hidden")]]
extern handle_table_t *new_handle_table();


static inline syn_handle_t invalid_hdl()
{
	const syn_handle_t hdl = {
		.generation = UINT32_MAX,
		.handle_matrix_index = UINT32_MAX,
		.addr = nullptr,
		.header = nullptr,
	};
	return hdl;
}


/// @brief Creates a new entry in the handle table wherever it fits.
///
/// @param head The head to link to the handle.
/// @return a _hopefully_ valid Arena Handle. handle->addr will be NULL if it fails.
[[gnu::visibility("hidden")]]
extern syn_handle_t create_handle_and_entry(pool_header_t * head);

#endif //ARENA_ALLOCATOR_HANDLE_H