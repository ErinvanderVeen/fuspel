#ifndef _H_FUSPEL_LEX
#define _H_FUSPEL_LEX

#include "syntax.h"

#define INITIAL_TOKEN_LIST_SIZE 20

struct token_list *lex(struct token_list*, char*);

#endif
