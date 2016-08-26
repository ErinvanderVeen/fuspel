#ifndef _H_EVAL
#define _H_EVAL

#include "syntax.h"

expression* eval_main(fuspel*);

expression* eval_rnf(fuspel*, expression*);
expression* eval(fuspel*, expression*);

#endif
