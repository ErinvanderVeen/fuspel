#ifndef _H_FUSPEL_MEM
#define _H_FUSPEL_MEM

#include <stdlib.h>

void *my_calloc(size_t num, size_t size);
void *my_realloc(void *ptr, size_t size);
void my_free(void *ptr);

#endif
