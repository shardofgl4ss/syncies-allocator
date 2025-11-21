#include "sync_alloc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <stdlib.h>
#include <time.h>
static constexpr int UPPER_LIMIT = 256;
static constexpr int LOWER_LIMIT = 32;

static constexpr u_int32_t SIZE = 64;
static constexpr char TEXTDATA[SIZE] =
	"meowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeo";

int main()
{
	// syn_handle_t new_hdl = syn_alloc((128 * 1024) - 16);
	// char *msg = syn_freeze(&new_hdl);
	//
	// memcpy(msg, TEXTDATA, SIZE);
	// msg[SIZE - 1] = '\0';
	// assert(!strcmp(msg, TEXTDATA));
	// printf("%s\n", msg);
	//
	// new_hdl = syn_thaw(msg);
	// syn_free(&new_hdl);
	// syn_destroy();

	constexpr u_int64_t runs = 256;
	srand(time(nullptr));
	for (int j = 0; j < runs; j++) {
		constexpr u_int64_t number_of_allocations = 1024;
		for (int i = 1; i < number_of_allocations >> 1; i++) {
			const size_t allocation_size =
				rand() % (UPPER_LIMIT + LOWER_LIMIT + 1) + LOWER_LIMIT;
			syn_handle_t hdl = syn_calloc(allocation_size + 1);

			char *heap_msg = syn_freeze(&hdl);
			memset(heap_msg, 'a', allocation_size);
			heap_msg[allocation_size] = '\0';

			assert((heap_msg[0] == 'a' &&
			        heap_msg[allocation_size - 1] == 'a' &&
			        heap_msg[allocation_size] == '\0'));

			hdl = syn_thaw(heap_msg);
			if (i & 1) {
				syn_free(&hdl);
			}
		}
		syn_reset();

		for (int i = 1; i < number_of_allocations >> 1; i++) {
			const size_t allocation_size =
				rand() % (UPPER_LIMIT + LOWER_LIMIT + 1) + LOWER_LIMIT;
			syn_handle_t hdl = syn_calloc(allocation_size + 1);

			char *heap_msg = syn_freeze(&hdl);
			memset(heap_msg, 'b', allocation_size);
			heap_msg[allocation_size] = '\0';

			assert(heap_msg[0] == 'b' &&
			       heap_msg[allocation_size - 1] == 'b' &&
			       heap_msg[allocation_size] == '\0');

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
