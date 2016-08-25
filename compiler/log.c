#include "log.h"

#include <stdio.h>

void log_debug(char* msg) {
	#if(LOG_LEVEL < LOG_DEBUG)
	fprintf(stdout, "%s\n", msg);
	#endif
}
