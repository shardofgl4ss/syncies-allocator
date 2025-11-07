#include "syn_memops.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static constexpr int upper_limit = 1000;
static constexpr int lower_limit = 10;

void test_memcpy()
{
	srand(time(nullptr));
	const int rand_value = rand() % ((upper_limit - lower_limit + 1) + lower_limit);
	char a[rand_value + 1];
	char b[rand_value + 1];
	for (int i = 0; i < rand_value; i++) {
		a[i] = 'a';
	}
	a[rand_value] = '\0';
	syn_memcpy(b, a, rand_value);
	b[rand_value] = '\0';
	if (strcmp(a, b) == 0) {
		printf("arrays are equal\n");
		fflush(stdout);
	}
	else {
		printf("arrays differ!\n");
	}
	assert(!strcmp(a, b));
}


void test_memset()
{
	srand(time(nullptr));
	const int rand_value = rand() % ((upper_limit - lower_limit + 1) + lower_limit);
	char a[rand_value + 1];
	char b[rand_value + 1];
	for (int i = 0; i < rand_value; i++) {
		a[i] = 'a';
	}
	a[rand_value] = '\0';
	syn_memset(b, 'a', rand_value);
	b[rand_value] = '\0';
	if (strcmp(a, b) == 0) {
		printf("arrays are equal\n");
		fflush(stdout);
	}
	else {
		printf("arrays differ!\n");
	}
	assert(!strcmp(a, b));
}


int main()
{
	test_memcpy();
	test_memset();
}
