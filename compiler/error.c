#include "error.h"

#include <stdio.h>
#include <stdlib.h>

void error(int code, char* message) {
	if (message) {
		fprintf(stderr, "%s\n", message);
	}

	exit(code);
}

void error_no_mem(void) {
	error(ERROR_NO_MEMORY, "No memory");
}
