//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "alloc_utils.h"
#include "deadzone.h"
#include "defs.h"
#include "free_node.h"
#include "globals.h"
#include "structs.h"
#include "types.h"
#include <stdint.h>

typedef struct Header_Context {
	memory_pool_t *pool;
	memory_pool_t **pool_array;
	pool_header_t **null_head;
	u32 num_bytes;
	u8 jump_table_index;
} __attribute__((aligned(32))) header_context_t;

static inline i32 zero_offset_header(const header_context_t *restrict ctx);
static inline i32 linear_offset_header(const header_context_t *restrict ctx);
static inline i32 free_list_header(const header_context_t *restrict ctx);
typedef i32 (*header_type_jumptable_t)(const header_context_t *restrict ctx);

header_type_jumptable_t header_jumptable[] = {
	zero_offset_header,
	free_list_header,
	linear_offset_header,
};

enum {
	ZERO_OFFSET   = 0,
	FREE_OFFSET   = 1,
	LINEAR_OFFSET = 2,
};

static constexpr u32 JUMPTABLE_LAST_INDEX = (sizeof(header_jumptable) / sizeof(header_jumptable[0])) - 1;
// clang-format off


static inline void create_head_sentinel(const header_context_t *restrict ctx)
{
	const pool_header_t empty = {};
	pool_header_t *restrict sentinel_head = (pool_header_t *)((char *)ctx->pool->mem + ctx->pool->offset);

	*sentinel_head = empty;
	sentinel_head->chunk_size = PD_HEAD_SIZE;
	sentinel_head->allocation_size = 0;
	sentinel_head->handle_matrix_index = 0;
	sentinel_head->bitflags |= F_SENTINEL | F_FROZEN;
}


static pool_header_t *create_header(const header_context_t *restrict ctx, const uintptr_t offset)
{
	if (ctx->jump_table_index == FREE_OFFSET) {
		goto skip_space_check;
	}
	const bool pool_has_no_space_left = (ctx->pool->size - offset)
	                                  < (ADD_ALIGNMENT_PADDING(ctx->num_bytes)
	                                    + PD_HEAD_SIZE
	                                    + (DEADZONE_PADDING * 2));

	// clang-format on
	if (pool_has_no_space_left) {
		return nullptr;
	}

skip_space_check:
	const pool_header_t empty_head = {};
	pool_header_t *restrict head = (pool_header_t *)((char *)ctx->pool->mem + offset);

	const bit32 prev_bits = (head->bitflags & F_FIRST_HEAD) ? F_FIRST_HEAD : 0;
	*head = empty_head;
	head->bitflags = prev_bits;

	const uintptr_t relative_alignment_offset = ALIGN_PTR(head, ALIGNMENT) - (uintptr_t)head;
	const u32 chunk_size = ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING + relative_alignment_offset;
	const u32 pad_chunk_size = ADD_ALIGNMENT_PADDING(chunk_size);

	head->allocation_size = ctx->num_bytes;
	head->chunk_size = pad_chunk_size;
	head->handle_matrix_index = 0;

	/* This is to clear the bitflags in case the header is being	*
	 * placed on a sentinel so it isn't inherited through casts.	*/

	head->bitflags |= (ctx->pool->offset == 0) ? (F_ALLOCATED | F_FIRST_HEAD) : F_ALLOCATED;

	create_head_deadzone(head, ctx->pool);

	// TODO figure out this bug

	if (offset == ctx->pool->offset) {
		ctx->pool->offset += pad_chunk_size;

		if (ctx->pool->offset + PD_HEAD_SIZE > ctx->pool->size) {
			head->bitflags |= F_SENTINEL; // head becomes sentinel if there is not enough space
			goto done;
		}
		create_head_sentinel(ctx);
	}
done:
	return head;
}


static inline i32 zero_offset_header(const header_context_t *restrict ctx)
{
	if (ctx->pool == nullptr || ctx->pool->offset != 0) {
		return 1;
	}
	*ctx->null_head = create_header(ctx, 0);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


static inline i32 linear_offset_header(const header_context_t *restrict ctx)
{
	const bool pool_is_invalid = (ctx->pool == nullptr || ctx->pool->offset == 0) != 0;
	if (pool_is_invalid) {
		return 1;
	}
	const bool pool_out_of_space = (ctx->pool->offset
	                                + ctx->num_bytes
	                                + PD_HEAD_SIZE
	                                + DEADZONE_PADDING > ctx->pool->size) != 0;
	if (pool_out_of_space) {
		return 1;
	}
	*ctx->null_head = create_header(ctx, ctx->pool->offset);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


static inline i32 free_list_header(const header_context_t *restrict ctx)
{
	if (ctx->pool->first_free == nullptr || ctx->pool->free_count == 0) {
		return 1;
	}

	pool_free_node_t *new_node = free_node_remove(ctx->pool, ctx->num_bytes);
	if (!new_node) {
		return 1;
	}
	*ctx->null_head = create_header(ctx, (intptr)new_node - (intptr)ctx->pool->mem);
	return 0;
}


static i32 find_new_header(header_context_t *restrict ctx)
{
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr) - 1;

	for (int i = 0; i <= pool_arr_len; i++) {
		ctx->pool = pool_arr[i];
		for (int j = 0; j <= JUMPTABLE_LAST_INDEX; j++) {
			ctx->jump_table_index = j;
			if (header_jumptable[j](ctx) == 0) {
				return j + 1; // + 1 so it starts at 1 instead of 0
			}
		}
	}
	return 0;
}


pool_header_t *find_or_create_new_header(const u32 requested_size)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || requested_size == 0) {
		return nullptr;
	}

	pool_header_t *new_head = nullptr;
	header_context_t ctx = {
		.pool = arena_thread->first_mempool,
		.pool_array = nullptr,
		.null_head = &new_head,
		.num_bytes = requested_size,
		.jump_table_index = 0,
	};

	const int ret = find_new_header(&ctx);
	if (ret == 0 || new_head == nullptr) {
		return nullptr;
	}
	update_sentinel_and_free_flags(new_head);
	return new_head;
}