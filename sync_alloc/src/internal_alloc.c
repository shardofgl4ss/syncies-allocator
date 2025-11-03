//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "alloc_utils.h"
#include "defs.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"

typedef struct header_ctx
{
	memory_pool_t *pool;
	pool_header_t **null_head;
	u32 num_bytes;
} header_ctx;

// forward declarations so the jump table can use them while still staying at the top,
// since these functions are local to this file.
inline static int zero_offset_header(const header_ctx *restrict ctx);

inline static int linear_offset_header(const header_ctx *restrict ctx);

inline static int free_list_header(const header_ctx *restrict ctx);

typedef int (*head_find_jt)(const header_ctx *restrict ctx);

head_find_jt header_jtable[] = {
	zero_offset_header,
	linear_offset_header,
	free_list_header,
};

static constexpr u32 jump_table_idx_l = (sizeof(header_jtable) / sizeof(header_jtable[0])) - 1;


pool_header_t *mempool_create_header(const header_ctx *restrict ctx, const intptr offset) {
	if ((ctx->pool->size - ctx->pool->offset) < (helper_add_padding(ctx->num_bytes) + PD_HEAD_SIZE)) {
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


inline static int zero_offset_header(const header_ctx *restrict ctx) {
	if (arena_thread == nullptr || ctx->pool == nullptr || ctx->pool->offset != 0) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, 0);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static int linear_offset_header(const header_ctx *restrict ctx) {
	if (arena_thread == nullptr || ctx->pool == nullptr || ctx->pool->offset == 0 ||
	    ctx->pool->offset + ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING > ctx->pool->size) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, ctx->pool->offset);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static int free_list_header(const header_ctx *restrict ctx) {
	if (arena_thread == nullptr || ctx->pool == nullptr || ctx->pool->free_count == 0) {
		return 1;
	}
	pool_free_header_t *free_arr[ctx->pool->free_count];
	const int free_arr_len = return_free_array(free_arr, ctx->pool);
	if (!free_arr_len) {
		return 1;
	}
	pool_free_header_t *header_candidate;
	pool_free_header_t *next_free = nullptr;
	pool_free_header_t *prev_free = nullptr;
	for (int i = 0; i < free_arr_len; i++) {
		header_candidate = free_arr[i];
		if (header_candidate->size == ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING) {
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
		if (header_candidate->size > ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING) {
			continue; // TODO add block splitting
		}
	}
	return 1;
done:
	*ctx->null_head = mempool_create_header(ctx, (intptr)header_candidate - (intptr)ctx->pool->mem);
	ctx->pool->free_count--;
	return 0;
}


int find_new_header(header_ctx *restrict ctx) {
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr) {
		return -1;
	}
	if (ctx->pool != arena_thread->first_mempool) {

	}
	memory_pool_t *pool_arr[arena_thread->pool_count];

	const int pool_arr_len = return_pool_array(pool_arr);
	int ret = 0;
	for (int i = 0; i < pool_arr_len; i++) {
		ctx->pool = pool_arr[i];

		/* The second and third functions in the jt can be 	*
		 * swapped, but zero_offset_header() should be first,	*
		 * as the other ones expect offset > 0.			*/

		for (int j = 0; j < jump_table_idx_l; j++) {
			ret = header_jtable[j](ctx);
			if (ret == 0) {
				goto done;
			}
		}
	}
	return 0;
done:
	return ret;
}


pool_header_t *find_or_create_new_pool_block(const u32 requested_size) {
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || requested_size == 0) {
		return nullptr;
	}

	/* -------------------------------------------------------------------------------------------- *
	 * if the quickest options for just checking offset and if it can fit in remaining space	*
	 * return successfully, then we just use those. Only when pools are full do we walk 		*
	 * free lists, because free list walking is (probably) slower than just immediately		*
	 * knowing you can put one.									*
	 * --------------------------------------------------------------------------------------------	*
	 * The way each pool is allocated is incremenetal, ie:						*
	 * if pool offset is zero, put the allocation there, else, skip,				*
	 * if pool size can fit the allocation, put it there, else, skip,				*
	 * if the pool has a free list, look for a freed header that can fit,				*
	 * else, move to next pool and repeat for all pools. If none can fit,				*
	 * create a new pool and retry with the new pool.						*
	 * -------------------------------------------------------------------------------------------- *
	 * also I might start using fancy comment blocks like these. they look nice.			*/

	pool_header_t *new_head = nullptr;
	header_ctx ctx = {
		.pool = arena_thread->first_mempool,
		.null_head = &new_head,
		.num_bytes = requested_size,
	};

	const int ret = find_new_header(&ctx);
	if (ret == 0 || new_head == nullptr) {
		return nullptr;
	}
	return new_head;
}
