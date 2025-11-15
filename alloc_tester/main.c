
#include "sync_alloc.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>

//#include "syn_memops.h"
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//static constexpr int upper_limit = 1000;
//static constexpr int lower_limit = 10;

//void test_memcpy();
//void test_memset();

int main() {
	const char *textdata = "meowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeowmeo";
	constexpr size_t allocation_size = 64;
	constexpr u_int64_t number_of_allocations = 64;
	for (int i = 1; i < number_of_allocations; i++) {
		syn_handle_t hdl = syn_calloc(allocation_size);

		char *heap_msg = syn_freeze(&hdl);
		memcpy(heap_msg, textdata, strlen(textdata) + 1);

		assert(!strcmp(textdata, heap_msg));
		hdl = syn_thaw(heap_msg);
		if (i == 4 || i == 12 || i == 17 || i == 31 || i == 52) {
			syn_free(&hdl);
		}
	}
	syn_reset();

	for (int i = 1; i < number_of_allocations; i++) {
		syn_handle_t hdl;
		if (i == 9) {
			hdl = syn_alloc(allocation_size);
		} else {
			hdl = syn_alloc(allocation_size);
		}

		char *heap_msg = syn_freeze(&hdl);
		memcpy(heap_msg, textdata, strlen(textdata) + 1);

		assert(!strcmp(textdata, heap_msg));
		hdl = syn_thaw(heap_msg);
		if (i == 4 || i == 12 || i == 17 || i == 31 || i == 52) {
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
