#ifndef _H_FUSPEL_GRAPHS
#define _H_FUSPEL_GRAPHS

#include <stdbool.h>
#include "syntax.h"

enum node_kind{
	NODE_INT,        /* See expr_kind in syntax.h */
	NODE_NAME,
	NODE_CODE,
	NODE_LIST,
	NODE_TUPLE,
	NODE_APP,

	NODE_REDIRECT    /* Redirect to another node */
};

struct node {
	enum node_kind kind;
	void *var1;
	void *var2;
	unsigned int used_count;
};

void use_node(struct node *node, unsigned int count);
void free_node(struct node *node, unsigned int count, bool free_first);

struct node ***flatten_app_args(struct node **from, bool remove_redirs);

void remove_redirects(struct node *node);

void cpy_expression_to_node(struct node *dst, struct expression *src);
void cpy_node_to_expression(struct expression *dst, struct node *src);

#endif
