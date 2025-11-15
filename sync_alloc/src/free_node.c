//
// Created by SyncShard on 11/15/25.
//

#include "free_node.h"
#include "structs.h"


int free_node_add(memory_pool_t *pool, pool_free_node_t *free_node)
{
	if (pool->first_free == nullptr) {
		pool->first_free = free_node;
		goto done;
	}
	// for now we just add new frees to the end. Might not be temporary.

	pool_free_node_t *current_node = pool->first_free;
	while (current_node->next_free != nullptr) {
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