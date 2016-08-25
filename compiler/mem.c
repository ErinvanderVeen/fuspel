#include "mem.h"

#include <stdio.h>

void* my_calloc(size_t num, size_t size) {
	void* ptr = calloc(num, size);
	if (!ptr) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void my_free(void* ptr) {
	free(ptr);
}
