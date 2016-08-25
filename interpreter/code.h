#ifndef _H_CODE
#define _H_CODE

#include "syntax.h"

typedef expression* (Code_0) ();
typedef expression* (Code_1) (expression*);
typedef expression* (Code_2) (expression*, expression*);

unsigned char code_find(char* name, void** function);

#endif
