//
// Created by SyncShard on 11/15/25.
//

#include "free_node.h"
#include "alloc_utils.h"
#include "defs.h"
#include "globals.h"
#include "structs.h"
#include "types.h"

//static constexpr u32 MAX_ADDED_CHUNK_SIZE = (ALIGNMENT + (DEADZONE_PADDING * 2));


static inline pool_free_node_t *index_free_list_end(pool_free_node_t *first_node, const u32 max_index)
{
	pool_free_node_t *node = first_node;
	for (int i = 0; i < max_index - 1; i++) {
		node = node->next_node;
	}
	return node;
}


static bool node_has_equal_size(const pool_free_node_t *node_current, const u32 allocation_size)
{
	const u32 pad_chunk_size = ADD_ALIGNMENT_PADDING(allocation_size + PD_HEAD_SIZE + DEADZONE_PADDING);
	if (node_current->chunk_size > pad_chunk_size) {
		return false;
	}
	if (node_current->chunk_size < pad_chunk_size) {
		return false;
	}
	return true;
}


static pool_free_node_t *index_free_list_size(pool_free_node_t **first_node,
                                              const u32 max_index,
                                              const u32 allocation_size)
{
	// holy moly if statements
	pool_free_node_t *node_current = *first_node;
	pool_free_node_t *node_next = nullptr;
	pool_free_node_t *node_prev = nullptr;

	if (node_has_equal_size(node_current, allocation_size)) {
		if (node_current->next_node) {
			node_next = node_current->next_node;
		}
		goto found;
	}

	for (int idx = 0; idx < max_index - 1; idx++) {
		if (node_current->next_node) {
			node_next = node_current->next_node;
		}

		if (node_has_equal_size(node_current, allocation_size)) {
			goto found;
		}

		node_prev = node_current;
		node_current = node_current->next_node;
	}
	if (!node_has_equal_size(node_current, allocation_size)) {
		return nullptr;
	}
found:
	// TODO block splitting
	if (node_next == node_current) {
		node_next = nullptr;
	}

	if (node_next && node_prev) {
		node_prev->next_node = node_next;
	} else if (node_next) {
		*first_node = node_next;
	} else if (node_prev) {
		node_prev->next_node = nullptr;
	} else {
		*first_node = nullptr;
	}

	return node_current;
}


int free_node_add(pool_free_node_t *free_node)
{
	memory_pool_t *pool = return_pool((pool_header_t *)free_node);
	if (pool->first_free == nullptr) {
		pool->first_free = free_node;
		goto done;
	}
	// for now we just add new frees to the end. Might not be temporary.

	pool_free_node_t *node = index_free_list_end(pool->first_free, pool->free_count);

	node->next_node = free_node;
done:
	pool->free_count++;
	return 0;
}


pool_free_node_t *free_node_remove(memory_pool_t *pool, const u32 size)
{
	pool_free_node_t *node = index_free_list_size(&pool->first_free, pool->free_count, size);
	if (node == nullptr) {
		return nullptr;
	}

	pool->free_count--;
	return node;
}


inline int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool)
{
	pool_free_node_t *head = pool->first_free;

	int idx = 0;
	while (idx < pool->free_count && head != nullptr) {
		arr[idx++] = head;
		head = head->next_node;
	}

	return idx;
}