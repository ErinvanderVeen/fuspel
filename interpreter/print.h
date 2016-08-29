#ifndef _H_PRINT
#define _H_PRINT

#include "syntax.h"
#include "graphs.h"

void print_token(token*);
void print_token_list(token_list*);

void print_expression(expression*);
void print_rewrite_rule(rewrite_rule*);
void print_fuspel(fuspel*);

void print_node(struct node*);

#endif
