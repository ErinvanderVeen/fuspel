#include "mem.h"

#include "error.h"

void* my_calloc(size_t num, size_t size) {
	void* ptr = calloc(num, size);
	if (!ptr)
		error_no_mem();
	return ptr;
}

void my_free(void* ptr) {
	free(ptr);
}
