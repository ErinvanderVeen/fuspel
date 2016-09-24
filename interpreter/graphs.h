#ifndef _H_GRAPHS
#define _H_GRAPHS

#include "syntax.h"

typedef enum {
	NODE_INT,        /* See expr_kind in syntax.h */
	NODE_NAME,
	NODE_CODE,
	NODE_LIST,
	NODE_TUPLE,
	NODE_APP,
} node_kind;

struct node {
	node_kind kind;
	void* var1;
	void* var2;
	unsigned int used_count;
};

void use_node(struct node* node, unsigned int count);
void free_node(struct node* node, unsigned int count, unsigned free_first);

void cpy_expression_to_node(struct node* dst, expression* src);
void cpy_node_to_expression(expression* dst, struct node* src);

#endif
