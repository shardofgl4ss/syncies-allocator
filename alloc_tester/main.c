#include "sync_alloc.h"
#include <stdio.h>

int
main(void)
{
	constexpr int z = 32;
	struct Arena_Handle hdl = syn_alloc(z);
	char *msg = syn_freeze(&hdl);

	printf("hello world1\n");
	scanf("%s", msg);
	printf("%s 1\n", msg);
	fflush(stdin);
	fflush(stdout);

	struct Arena_Handle hdl2 = syn_alloc(z * 2);
	char *msg2 = syn_freeze(&hdl2);

	printf("hello world2\n");
	scanf("%s", msg2);

	printf("%s 2\n", msg2);
	printf("%s 1\n", msg);
	fflush(stdout);


	struct Arena_Handle hdl3 = syn_alloc(z * 2);
	char *msg3 = syn_freeze(&hdl3);

	printf("hello world3\n");
	scanf("%s", msg3);

	printf("%s 3\n", msg3);
	printf("%s 2\n", msg2);
	printf("%s 1\n", msg);
	fflush(stdout);

	syn_thaw(&hdl2);
	syn_free(&hdl2);
	struct Arena_Handle hdl4 = syn_alloc(z);
	char *msg4 = syn_freeze(&hdl4);

	printf("hello world4\n");
	scanf("%s", msg4);

	printf("%s 4\n", msg4);
	printf("%s 3\n", msg3);
	printf("%s 1\n", msg);
	fflush(stdout);

	syn_thaw(&hdl);
	syn_free_all();
}
