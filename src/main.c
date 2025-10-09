#include "../include/sync-alloc.h"


int
main(void)
{
	Arena *arena = arena_create();
	if (!arena) return 0;

	char *test = arena_alloc(arena, 64);
	if (!test) return 0;


	fgets(test, 64, stdin);

	printf("%s\n", test);
	arena_destroy(arena);
}