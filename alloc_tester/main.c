#include "sync_alloc.h"
#include <stdio.h>


int
main(void)
{

	for (int i = 0; i < 10; i++)
	{
		printf("iteration %d\n", i);

		constexpr int z = 32;
		struct Arena_Handle hdl = syn_alloc(z);
		char *msg = syn_freeze(&hdl);

		msg = "beepbeepbeepbeepbeepbeepbeepbee\n";
		printf("%s 1\n", msg);

		syn_thaw(&hdl);
		struct Arena_Handle hdl2 = syn_alloc(z * 2);
		char *msg2 = syn_freeze(&hdl2);

		printf("hello world2\n");
		msg2 = "beepbeepbeepbeepbeepbeepbeepbeep\n";

		printf("%s 2\n", msg2);


		syn_thaw(&hdl2);
		struct Arena_Handle hdl3 = syn_alloc(z * 2);
		char *msg3 = syn_freeze(&hdl3);

		printf("hello world3\n");

		msg3 = "beepbeepbeepbeepbeepbeepbeepbeep\n";
		printf("%s 3\n", msg3);

		syn_thaw(&hdl3);
		struct Arena_Handle hdl4 = syn_alloc(z);
		char *msg4 = syn_freeze(&hdl4);

		printf("hello world4\n");
		msg4 = "beepbeepbeepbeepbeepbeepbeepbeep\n";

		printf("%s 4\n", msg4);

		syn_thaw(&hdl4);
		syn_free_all();
	}
}
