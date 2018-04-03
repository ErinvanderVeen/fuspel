#ifndef _H_FUSPEL_EVAL
#define _H_FUSPEL_EVAL

#include <stdio.h>

#include "syntax.h"

struct expression *eval_main(FILE *out, struct fuspel*
#ifdef FUSPEL_DEBUG
		, bool debug_graphs
#endif
		);

#endif
