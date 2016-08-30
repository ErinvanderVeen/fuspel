#ifndef _H_GRAPHS
#define _H_GRAPHS

#include "syntax.h"

struct node {
	expr_kind kind;
	void* var1;
	void* var2;
	unsigned int used_count;
};

typedef struct {
	unsigned int length;
	struct node* nodes[1];
} nodes_array;

void use_node(struct node* node, unsigned int count);
void free_node(struct node* node, unsigned int count, unsigned free_first);

nodes_array* push_node(nodes_array* nodes, struct node* node);

void cpy_expression_to_node(struct node* dst, expression* src);
void cpy_node_to_expression(expression* dst, struct node* src);

#endif
