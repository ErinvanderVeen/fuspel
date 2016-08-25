#include "log.h"

#include <stdio.h>

#if(LOG_LEVEL < LOG_DEBUG)
void log_debug(char* msg) {
	fprintf(stdout, "%s\n", msg);
}
#endif
