//
// Created by SyncShard on 10/9/25.
//

#include "internal_alloc.h"
#include "defs.h"
#include "helper_functions.h"
#include "structs.h"
#include "types.h"

typedef struct header_ctx {
	Memory_Pool *pool;
	Pool_Header **null_head;
	u32 num_bytes;
} header_ctx;

// forward declarations so the jump table can use them while still staying at the top,
// since these functions are local to this file.
inline static int zero_offset_header(header_ctx *ctx);
inline static int linear_offset_header(header_ctx *ctx);
inline static int free_list_header(header_ctx *ctx);
typedef int (*head_find_jt)(header_ctx *ctx);

head_find_jt header_jtable[] = {
		zero_offset_header,
		free_list_header,
		linear_offset_header,
};


Pool_Header *mempool_create_header(header_ctx *restrict ctx, const intptr offset)
{
	if ((ctx->pool->pool_size - ctx->pool->pool_offset) < (helper_add_padding(ctx->num_bytes) + PD_HEAD_SIZE)) {
		return nullptr;
	}

	// just for show, the way this is added is actually the memory layout of each header-alloc-pad.
	const u32 chunk_size = (u32)helper_add_padding(PD_HEAD_SIZE + ctx->num_bytes + DEADZONE_PADDING);
	auto *head = (Pool_Header *)((char *)ctx->pool->mem + offset);

	head->size = chunk_size;
	head->handle_idx = 0;
	head->block_flags |= PH_ALLOCATED;

	// TODO implement split deadzone that has a u32 for the previous block size stored in its upper half.
	auto *deadzone = (u64 *)((char *)head + chunk_size - DEADZONE_PADDING);
	*deadzone = POOL_DEADZONE;

	ctx->pool->pool_offset += chunk_size;
	return head;
}


inline static int return_pool_array(Memory_Pool **arr)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || arr == nullptr) {
		return 0;
	}
	Memory_Pool *pool = arena_thread->first_mempool;

	int idx = 0;
	while (idx < arena_thread->pool_count && pool != nullptr) {
		arr[idx++] = pool;
		pool = pool->next_pool;
	}
	return idx;
}


inline static int return_free_array(Pool_Free_Header **arr, const Memory_Pool *pool)
{
	if (arena_thread == nullptr || pool == nullptr || pool->first_free == nullptr || arr == nullptr) {
		return 0;
	}
	Pool_Free_Header *head = pool->first_free;

	int idx = 0;
	while (idx < pool->free_count && head != nullptr) {
		arr[idx++] = head;
		head = head->next_free;
	}
	return idx;
}


inline static int zero_offset_header(header_ctx *ctx)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || ctx->pool->pool_offset != 0) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, 0);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static int linear_offset_header(header_ctx *ctx)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || ctx->pool->pool_offset == 0 ||
	    ctx->pool->pool_offset + ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING > ctx->pool->pool_size) {
		return 1;
	}
	*ctx->null_head = mempool_create_header(ctx, ctx->pool->pool_offset);
	if (*ctx->null_head == nullptr) {
		return 1;
	}
	return 0;
}


inline static int free_list_header(header_ctx *ctx)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || ctx->pool->free_count == 0) {
		return 1;
	}
	Pool_Free_Header *arr[ctx->pool->free_count];
	Pool_Free_Header *header_candidate;
	const int free_arr_len = return_free_array(arr, ctx->pool);
	if (!free_arr_len) {
		return 1;
	}
	for (int i = 0; i < free_arr_len; i++) {
		header_candidate = arr[i];
		if (header_candidate->size >= ctx->num_bytes + PD_HEAD_SIZE + DEADZONE_PADDING) {
			goto done;
		}
	}
	return 1;
done:
	*ctx->null_head = mempool_create_header(ctx, (intptr)header_candidate - (intptr)ctx->pool->mem);
	return 0;
}


int find_new_header(header_ctx *ctx)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr) {
		return -1;
	}
	Memory_Pool *pool_arr[arena_thread->pool_count];
	constexpr u32 jump_table_idx_l = (sizeof(header_jtable) / sizeof(header_jtable[0])) - 1;

	const int pool_arr_len = return_pool_array(pool_arr);
	int ret = 0;
	for (int i = 0; i < pool_arr_len; i++) {
		ctx->pool = pool_arr[i];
		
		/* The second and third functions here can be swapped,	*
		 * but zero_offset_header() should be first,		*
		 * as the other ones expect an offset > 0.		*/

		for (int j = 0; j < jump_table_idx_l; j++) {
			ret = header_jtable[j](ctx);
		}
		if (ret != 0) {
			break;
		}
	}
	return ret;
}


Pool_Header *find_new_pool_block(const u32 requested_size)
{
	if (arena_thread == nullptr || arena_thread->first_mempool == nullptr || requested_size == 0) {
		return nullptr;
	}

	/* if the quickest options for just checking offset and if it can fit in remaining space	*
	 * return successfully, then we just use those. Only when pools are full do we walk free lists.	*
	 * cause free list walking is (probably) slower than just immediately knowing you can put one.	*/
	
	Pool_Header *new_head = nullptr;
	header_ctx ctx = {
		.pool = arena_thread->first_mempool,
		.null_head = &new_head,
		.num_bytes = requested_size,
	};

	const int ret = find_new_header(&ctx);
	if(ret == 0 || new_head == nullptr) {
		return nullptr;
	}
	return new_head;
}
