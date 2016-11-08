#ifndef _H_FUSPEL_PRINT
#define _H_FUSPEL_PRINT

#include "syntax.h"
#include "graphs.h"

#include <stdio.h>

void print_token(struct token*);
void print_token_list(struct token_list*);

void print_expression(struct expression*);
void print_rewrite_rule(struct rewrite_rule*);
void print_fuspel(struct fuspel*);

void print_node(struct node*);

#ifdef FUSPEL_DEBUG
struct visited_nodes {
	struct node *node;
	struct visited_nodes *next;
};

void print_node_to_file(struct node*, FILE*, struct visited_nodes*);
#endif

#endif
