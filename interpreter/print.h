#ifndef _H_PRINT
#define _H_PRINT

#include "syntax.h"
#include "graphs.h"

#include <stdio.h>

void print_token(token*);
void print_token_list(token_list*);

void print_expression(expression*);
void print_rewrite_rule(rewrite_rule*);
void print_fuspel(fuspel*);

void print_node(struct node*);

#ifdef _FUSPEL_DEBUG
void print_node_to_file(struct node*, FILE*);
#endif

#endif
