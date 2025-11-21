//
// Created by SyncShard on 10/15/25.
//

#ifndef ARENA_ALLOCATOR_HANDLE_H
#define ARENA_ALLOCATOR_HANDLE_H

#include "defs.h"
#include "globals.h"
#include "structs.h"
#include "sync_alloc.h"
#include "types.h"

// clang-format off

/**
 * @brief Handle generation checksum.
 * @param hdl The handle to verify
 * @return Returns true if they do not match, false if they don't.
 */
extern bool handle_generation_checksum(const syn_handle_t *restrict hdl);

/** Updates the entry's generation. */
extern void update_table_generation(u32 encoded_matrix_index);

extern void table_destructor();

extern int return_table_array(handle_table_t **arr);

extern syn_handle_t *return_handle(u32 encoded_matrix_index);

/**
 * 	Table of user allocations.
 *
 *	@details Each handle table has a max handle count of 64, allocated in chunks.
 *	Each table is in a linked list to allow easy infinite table allocation.
 *	The maximum capacity of a handle table is defined as TABLE_MAX_COL.
 *
 *	@note handle_entries[] might be converted into a fixed 64 array instead
 *	of a FAM, but for possible future dynamic expansion of handle tables,
 *	it will stay as this until I make up my mind.
 *
 *	@details PD_HANDLE_MATRIX_SIZE is directly equivalent to each total table size.
 *
 *	@details Bitmap:  0 == FREE, 1 == ALLOCATED.
 */
typedef struct Handle_Table {
	struct Handle_Table *next_table;	/**< Pointer to the next table.				*/
	bit64 entries_bitmap;			/**< Bitmap of used and free handles			*/
	u32 table_id;				/**< The index of the current table.			*/
	syn_handle_t handle_entries[];		/**< array of entries via FAM. index via entries bit.	*/
} handle_table_t;

// clang-format on

/// @brief Creates a new handle table. Does not increment table_count by itself, do that before calling.
///
/// @return a valid handle table if there is enough system memory.
extern handle_table_t *new_handle_table();

/// @brief Creates a new entry in the handle table wherever it fits.
///
/// @param head The head to link to the handle.
/// @return a _hopefully_ valid Arena Handle. handle->addr will be NULL if it fails.
extern syn_handle_t create_handle_and_entry(pool_header_t *head);

static constexpr u32 STRUCT_SIZE_HANDLE = sizeof(syn_handle_t);
static constexpr u32 STRUCT_SIZE_HANDLE_TABLE = sizeof(handle_table_t);
static constexpr u32 STRUCT_SIZE_HANDLE_MATRIX =
	(STRUCT_SIZE_HANDLE * MAX_TABLE_HNDL_COLS) + STRUCT_SIZE_HANDLE_TABLE;

#endif //ARENA_ALLOCATOR_HANDLE_H
