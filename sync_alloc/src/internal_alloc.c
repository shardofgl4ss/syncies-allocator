//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "alloc_utils.h"
#include "defs.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"

typedef struct Header_Context {
	memory_pool_t *pool;
	memory_pool_t **pool_array;
	pool_header_t **null_head;
	u32 num_bytes;
} header_context_t;

inline static i32 zero_offset_header(const header_context_t *restrict ctx);
inline static i32 linear_offset_header(const header_context_t *restrict ctx);
inline static i32 free_list_header(const header_context_t *restrict ctx);
typedef i32 (*header_type_jumptable_t)(const header_context_t *restrict ctx);

/* This is in order from top to bottom which will be tried first.	*
 * The second and third functions in the jt can be 			*
 * swapped, but zero_offset_header() should be first,			*
 * as the other ones expect offset > 0.					*/

header_type_jumptable_t header_jumptable[] = {
	zero_offset_header,
	linear_offset_header,
	free_list_header,
};

static constexpr u32 jumptable_last_index = (sizeof(header_jumptable) / sizeof(header_jumptable[0])) - 1;


pool_header_t *mempool_create_header(const header_context_t *restrict ctx, const intptr offset)
{
	const bool pool_has_no_space_left = (ctx->pool->size - ctx->pool->offset)
	                                 < (helper_add_padding(ctx->num_bytes)
	                                    + PD_HEAD_SIZE
	                                    + DEADZONE_PADDING);
	if (pool_has_no_space_left) {
		return nullptr;
	}

	// just for show, the way this is added is actually the memory layout of each header-alloc-pad.
	const u32 chunk_size = (u32)helper_add_padding(PD_HEAD_SIZE + ctx->num_bytes + DEADZONE_PADDING);
	auto *head = (pool_header_t *)((char *)ctx->pool->mem + offset);

	head->size = chunk_size;
	head->handle_idx = 0;
	head->bitflags |= PH_ALLOCATED;

	auto *deadzone = (u64 *)((char *)head + chunk_size - DEADZONE_PADDING);
	*deadzone = HEAD_DEADZONE;
	deadzone = deadzone + (DEADZONE_PADDING / 2);
	*deadzone = head->size;

	ctx->pool->offset += chunk_size;
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
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static i32 linear_offset_header(const header_context_t *restrict ctx)
{
	if (ctx->pool == nullptr || ctx->pool->offset == 0 || ctx->pool->offset + ctx->num_bytes + PD_HEAD_SIZE +
	    DEADZONE_PADDING > ctx->pool->size) {
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
		for (int j = 0; j < jumptable_last_index; j++) {
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
	};

	const int ret = find_new_header(&ctx);
	if (ret == 0 || new_head == nullptr) {
		return nullptr;
	}
	return new_head;
}
