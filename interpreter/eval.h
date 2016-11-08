#ifndef _H_FUSPEL_EVAL
#define _H_FUSPEL_EVAL

#include "syntax.h"

struct expression *eval_main(struct fuspel*
#ifdef FUSPEL_DEBUG
		, bool debug_graphs
#endif
		);

#endif
