#include "sync_alloc.h"
#include <stdio.h>

int
main(void)
{
	const int x = arena_create();
	if (x == 1)
		return 1;
	constexpr int z = 32;
	struct Arena_Handle hdl = arena_alloc(z);
	char *msg = handle_lock(&hdl);

	printf("hello world1\n");
	scanf("%s", msg);
	printf("%s 1\n", msg);
	fflush(stdin);
	fflush(stdout);

	struct Arena_Handle hdl2 = arena_alloc(z * 2);
	char *msg2 = handle_lock(&hdl2);

	printf("hello world2\n");
	scanf("%s", msg2);

	printf("%s 2\n", msg2);
	printf("%s 1\n", msg);
	fflush(stdout);
	//handle_unlock(&hdl2);


	struct Arena_Handle hdl3 = arena_alloc(z * 2);
	char *msg3 = handle_lock(&hdl3);

	printf("hello world3\n");
	scanf("%s", msg3);

	printf("%s 3\n", msg3);
	printf("%s 2\n", msg2);
	printf("%s 1\n", msg);
	fflush(stdout);
	//handle_unlock(&hdl3);

	handle_unlock(&hdl2);
	arena_free(&hdl2);
	struct Arena_Handle hdl4 = arena_alloc(z);
	char *msg4 = handle_lock(&hdl4);

	printf("hello world4\n");
	scanf("%s", msg4);

	printf("%s 4\n", msg4);
	printf("%s 3\n", msg3);
	printf("%s 1\n", msg);
	fflush(stdout);
	//handle_unlock(&hdl4);


	handle_unlock(&hdl);
	arena_destroy();
}