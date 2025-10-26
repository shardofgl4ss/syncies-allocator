#include "sync_alloc.h"
#include <stdio.h>

int
main(void)
{
	const int x = arena_create();
	if (x == 1)
		return 1;
	printf("arena created\n");
	struct Arena_Handle a = arena_alloc(256);
	printf("allocated 256 bytes to arena\n");
	char *b = handle_lock(&a);
	printf("locked handle\n");

	printf("enter something\n");
	fgets(b, 256, stdin);
	printf("%s\n", b);

	int y = arena_realloc(&a, 512);
	if (y == 1) {
		printf("realloc fail!\n");
	}

	handle_unlock(&a);
	printf("unlocked handle\n");

	y = arena_realloc(&a, 512);
	if (y == 0) {
		printf("realloc success!\n");
	}
	b = handle_lock(&a);
	printf("locked handle\n");
	printf("%s\n", b);
	handle_unlock(&a);
	printf("unlocked handle\n");

	arena_destroy();
}