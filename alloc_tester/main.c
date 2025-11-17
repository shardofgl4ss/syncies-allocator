#include "sync_alloc.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

static constexpr int upper_limit = 512;
static constexpr int lower_limit = 32;


int main()
{
	constexpr u_int64_t runs = 256;
	srand(time(nullptr));
	//const char *textdata = "meowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeo";
	for (int j = 0; j < runs; j++) {
		constexpr u_int64_t number_of_allocations = 1024;
		for (int i = 1; i < number_of_allocations / 2; i++) {
			const size_t allocation_size = rand() % (upper_limit + lower_limit + 1) + lower_limit;
			syn_handle_t hdl = syn_calloc(allocation_size + 1);

			char *heap_msg = syn_freeze(&hdl);
			memset(heap_msg, 'a', allocation_size);
			heap_msg[allocation_size] = '\0';

			assert((heap_msg[0] == 'a' && heap_msg[allocation_size - 1] == 'a' && heap_msg[allocation_size] == '\0'));
			hdl = syn_thaw(heap_msg);
			if (i & 1) {
				syn_free(&hdl);
			}
		}
		syn_reset();

		for (int i = 1; i < number_of_allocations / 2; i++) {
			const size_t allocation_size = rand() % (upper_limit + lower_limit + 1) + lower_limit;
			syn_handle_t hdl = syn_calloc(allocation_size + 1);

			char *heap_msg = syn_freeze(&hdl);
			memset(heap_msg, 'b', allocation_size);
			heap_msg[allocation_size] = '\0';

			assert(heap_msg[0] == 'b' && heap_msg[allocation_size - 1] == 'b' && heap_msg[allocation_size] == '\0');
			//assert(!strcmp(textdata, heap_msg));
			hdl = syn_thaw(heap_msg);
			if (i % 2) {
				syn_free(&hdl);
			}
		}
		syn_destroy();
	}
}


//void test_memcpy()
//{
//	srand(time(nullptr));
//	const int rand_value = rand() % ((upper_limit - lower_limit + 1) + lower_limit);
//	char a[rand_value + 1];
//	char b[rand_value + 1];
//	for (int i = 0; i < rand_value; i++) {
//		a[i] = 'a';
//	}
//	a[rand_value] = '\0';
//	syn_memcpy(b, a, rand_value);
//	b[rand_value] = '\0';
//	if (strcmp(a, b) == 0) {
//		printf("arrays are equal\n");
//		fflush(stdout);
//	}
//	else {
//		printf("arrays differ!\n");
//	}
//	assert(!strcmp(a, b));
//}
//
//
//void test_memset()
//{
//	srand(time(nullptr));
//	const int rand_value = rand() % ((upper_limit - lower_limit + 1) + lower_limit);
//	char a[rand_value + 1];
//	char b[rand_value + 1];
//	for (int i = 0; i < rand_value; i++) {
//		a[i] = 'a';
//	}
//	a[rand_value] = '\0';
//	syn_memset(b, 'a', rand_value);
//	b[rand_value] = '\0';
//	if (strcmp(a, b) == 0) {
//		printf("arrays are equal\n");
//		fflush(stdout);
//	}
//	else {
//		printf("arrays differ!\n");
//	}
//	assert(!strcmp(a, b));
//}