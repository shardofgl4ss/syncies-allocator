
#include "sync_alloc.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

static constexpr int upper_limit = 1024;
static constexpr int lower_limit = 32;

int main() {
	srand(time(nullptr));
	//const char *textdata = "meowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeo";
	constexpr u_int64_t number_of_allocations = 64;
	for (int i = 1; i < number_of_allocations; i++) {
		const size_t allocation_size = rand() % ((upper_limit + lower_limit + 1) + lower_limit);
		syn_handle_t hdl = syn_calloc(allocation_size);

		char *heap_msg = syn_freeze(&hdl);
		memset(heap_msg, 'a', allocation_size);

		//assert(!strcmp(textdata, heap_msg));
		hdl = syn_thaw(heap_msg);
		if (i & 1) {
			if (i == 63 || i == 33) {
				syn_free(&hdl);
			} else {
				syn_free(&hdl);
			}
		}
	}
	syn_reset();

	for (int i = 1; i < number_of_allocations; i++) {
		const size_t allocation_size = rand() % ((upper_limit + lower_limit + 1) + lower_limit);
		syn_handle_t hdl = {};

		hdl = syn_alloc(allocation_size);
		char *heap_msg = syn_freeze(&hdl);
		memset(heap_msg, 'b', allocation_size);

		//assert(!strcmp(textdata, heap_msg));
		hdl = syn_thaw(heap_msg);
		if (i % 2) {
			syn_free(&hdl);
		}
	}
	syn_destroy();
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
