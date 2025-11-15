//
// Created by SyncShard on 11/15/25.
//

#include "free_node.h"
#include "structs.h"


inline int return_free_array(pool_free_node_t **arr, const memory_pool_t *pool)
{
	pool_free_node_t *head = pool->first_free;

	int idx = 0;
	while (idx < pool->free_count && head != nullptr) {
		arr[idx++] = head;
		head = head->next_free;
	}

	return idx;
}


int free_node_add(memory_pool_t *pool, pool_free_node_t *free_node)
{
	if (pool->first_free == nullptr) {
		pool->first_free = free_node;
		goto done;
	}
	// for now we just add new frees to the end. Might not be temporary.

	pool_free_node_t *current_node = pool->first_free;
	for (int i = 0; i < pool->free_count - 1; i++) {
		//if (current_node->next_free == nullptr) {
		//	break;
		//}
		current_node = current_node->next_free;
	}

	current_node->next_free = free_node;
done:
	pool->free_count++;
	return 0;
}


// TODO implement free_node_remove(),
//int free_node_remove(memory_pool_t *pool, pool_free_node_t *free_node)
//{
//	return 0;
//}