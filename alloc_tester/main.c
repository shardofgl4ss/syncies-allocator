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

	handle_unlock(&a);
	printf("unlocked handle\n");
	arena_destroy();
}