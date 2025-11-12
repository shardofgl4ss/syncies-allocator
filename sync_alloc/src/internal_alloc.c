//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "alloc_utils.h"
#include "defs.h"
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

inline static i32 zero_offset_header(const header_context_t *restrict ctx);
inline static i32 linear_offset_header(const header_context_t *restrict ctx);
inline static i32 free_list_header(const header_context_t *restrict ctx);
typedef i32 (*header_type_jumptable_t)(const header_context_t *restrict ctx);

header_type_jumptable_t header_jumptable[] = {
	zero_offset_header,
	free_list_header,
	linear_offset_header,
};

enum {
	ZERO_OFFSET = 0,
	FREE_OFFSET = 1,
	LINEAR_OFFSET = 2,
};

static constexpr u32 JUMPTABLE_LAST_INDEX = (sizeof(header_jumptable) / sizeof(header_jumptable[0])) - 1;
// clang-format off

pool_header_t *mempool_create_header(const header_context_t *restrict ctx, const uintptr_t offset)
{
	if (ctx->jump_table_index != LINEAR_OFFSET) {
		goto skip_space_check;
	}
	const bool pool_has_no_space_left = (ctx->pool->size - offset)
	                                  < (ADD_ALIGNMENT_PADDING(ctx->num_bytes)
	                                    + PD_HEAD_SIZE
	                                    + DEADZONE_PADDING);

	// clang-format on
	if (pool_has_no_space_left) {
		return nullptr;
	}

skip_space_check:
	pool_header_t *head = (pool_header_t *)((u8 *)ctx->pool->mem + offset);

	const uintptr_t relative_alignment_offset = ALIGN_PTR(head, ALIGNMENT) - (uintptr_t)head;
	const u32 chunk_size = ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING + relative_alignment_offset;
	const u32 pad_chunk_size = ADD_ALIGNMENT_PADDING(chunk_size);

	head->allocation_size = ctx->num_bytes;
	head->chunk_size = pad_chunk_size;
	head->handle_idx = 0;
	head->bitflags |= (ctx->pool->offset == 0)
		? F_ALLOCATED | F_FIRST_HEAD
		: F_ALLOCATED;

	u32 *deadzone = (u32 *)((u8 *)head + (pad_chunk_size - DEADZONE_PADDING));
	*deadzone++ = HEAD_DEADZONE;
	*deadzone = head->chunk_size;

	if (offset == ctx->pool->offset) {
		ctx->pool->offset += pad_chunk_size;

		if (ctx->pool->offset + PD_HEAD_SIZE > ctx->pool->size) {
			head->bitflags |= F_SENTINEL; // head becomes sentinel if there is not enough space
			goto done;
		}

		pool_header_t *sentinel_head = (pool_header_t *)((u8 *)ctx->pool->mem + ctx->pool->offset);

		sentinel_head->chunk_size = PD_HEAD_SIZE;
		sentinel_head->allocation_size = 0;
		sentinel_head->handle_idx = 0;
		sentinel_head->bitflags |= F_SENTINEL | F_FROZEN;
	}
done:
	return head;
}


//static pool_free_header_t *detach_free_node(header_context *restrict ctx, pool_free_header_t **free_array) {
//}


inline static i32 zero_offset_header(const header_context_t *restrict ctx)
{
	if (ctx->pool == nullptr || ctx->pool->offset != 0) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, 0);
	if (*ctx[0].null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static i32 linear_offset_header(const header_context_t *restrict ctx)
{
	const bool pool_is_invalid = (ctx->pool == nullptr || ctx->pool->offset == 0) != 0;
	if (pool_is_invalid) {
		return 1;
	}
	const bool pool_out_of_space = (ctx->pool->offset + ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING > ctx->pool->size) != 0;
	if (pool_out_of_space) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, ctx->pool->offset);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static i32 free_list_header(const header_context_t *restrict ctx)
{
	if (ctx->pool == nullptr || ctx->pool->free_count == 0) {
		return 1;
	}
	pool_free_node_t *free_arr[ctx->pool->free_count];
	const int free_arr_len = return_free_array(free_arr, ctx->pool);
	if (!free_arr_len) {
		return 1;
	}
	pool_free_node_t *header_candidate;
	pool_free_node_t *next_free = nullptr;
	pool_free_node_t *prev_free = nullptr;
	const u32 chunk_size = ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING;
	for (int i = 0; i < free_arr_len; i++) {
		header_candidate = free_arr[i];
		// TODO extract free node detachment into its own function for modularity
		if (header_candidate->size < chunk_size) {
			continue;
		}
		if (header_candidate->size > chunk_size) {
			continue; // TODO add block splitting
		}
		if (header_candidate->next_free && i < free_arr_len - 1) {
			next_free = free_arr[i + 1];
		}
		if (i != 0) {
			prev_free = free_arr[i - 1];
		}
		if (next_free && prev_free) {
			prev_free->next_free = next_free;
		}
		if (next_free && !prev_free) {
			ctx->pool->first_free = next_free;
		}
		goto done;
	}
	return 1;
done:
	*ctx->null_head = mempool_create_header(ctx, (intptr)header_candidate - (intptr)ctx->pool->mem);
	ctx->pool->free_count--;
	return 0;
}


i32 find_new_header(header_context_t *restrict ctx)
{
	memory_pool_t *pool_arr[arena_thread->pool_count];
	const int pool_arr_len = return_pool_array(pool_arr);

	for (int i = 0; i < pool_arr_len; i++) {
		ctx->pool = pool_arr[i];
		for (int j = 0; j < JUMPTABLE_LAST_INDEX; j++) {
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

