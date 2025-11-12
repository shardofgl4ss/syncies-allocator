//
// Created by SyncShard on 10/9/25.
//

#ifndef ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
#define ARENA_ALLOCATOR_HELPER_FUNCTIONS_H

#include "defs.h"
#include "handle.h"
#include "structs.h"
#include "types.h"

/// @brief Calculates a handle's row via division.
/// @param hdl The handle to get the row from.
/// @return the handle index's row.
[[maybe_unused, gnu::pure, nodiscard, deprecated("use index_table function")]]
static inline u32 matrix_row_from_handle(const syn_handle_t *restrict hdl)
{
	return hdl->handle_matrix_index / MAX_TABLE_HNDL_COLS;
}


/// @brief Calculates a handle's column via modulo.
/// @param hdl The handle to get the column from.
/// @return the handle index's column.
[[maybe_unused, gnu::pure, nodiscard, deprecated("use index_table function")]]
static inline u32 matrix_col_from_handle(const syn_handle_t *restrict hdl)
{
	return hdl->handle_matrix_index % MAX_TABLE_HNDL_COLS;
}

#endif // ARENA_ALLOCATOR_HELPER_FUNCTIONS_H
