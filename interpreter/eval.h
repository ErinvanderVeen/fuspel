#ifndef _H_EVAL
#define _H_EVAL

#include "syntax.h"

struct expression *eval_main(struct fuspel*
#ifdef _FUSPEL_DEBUG
		, bool debug_graphs
#endif
		);

#endif
