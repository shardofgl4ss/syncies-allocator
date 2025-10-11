#include "sync_alloc/include/sync_alloc.h"

int
main(void)
{
	Arena *arena = arena_create();
	if (!arena) return 0;

	Arena_Handle hdl = arena_alloc(arena, 64);
	char *test = handle_lock(&hdl);
	if (!test) return 0;

	fgets(test, 64, stdin);

	printf("%s\n", test);
	arena_debug_print_memory_usage(arena);
	arena_destroy(arena);
}