#include "graphs.h"

#include <string.h>

#include "mem.h"

void use_node(struct node* node, unsigned int count) {
	if (!node)
		return;

	node->used_count += count;

	if (node->kind == NODE_LIST ||
			node->kind == NODE_TUPLE ||
			node->kind == NODE_APP) {
		use_node(node->var1, count);
		use_node(node->var2, count);
	}
}

void free_node(struct node* node, unsigned int count, unsigned free_first) {
	if (!node)
		return;

	node->used_count -= count;

	if (node->kind == NODE_LIST ||
			node->kind == NODE_TUPLE ||
			node->kind == NODE_APP) {
		free_node((struct node*) node->var1, count, 1);
		free_node((struct node*) node->var2, count, 1);
	}

	if (node->used_count == 0) {
		if (node->kind == NODE_INT || node->kind == NODE_NAME)
			my_free(node->var1);

		if (node->kind == NODE_CODE)
			my_free(node->var2);

		if (free_first)
			my_free(node);
	}
}

void cpy_expression_to_node(struct node* dst, expression* src) {
	if (!dst || !src)
		return;

	dst->kind = src->kind;
	switch (src->kind) {
		case EXPR_INT:
			dst->var1 = my_calloc(1, sizeof(int));
			*((int*) dst->var1) = *((int*) src->var1);
			break;

		case EXPR_NAME:
			dst->var1 = my_calloc(1, 1 + strlen((char*) src->var1));
			strcpy(dst->var1, src->var1);
			break;

		case EXPR_CODE:
			dst->var1 = src->var1;
			dst->var2 = my_calloc(1, sizeof(unsigned char));
			*((unsigned char*) dst->var2) = *((unsigned char*) src->var2);
			break;

		case EXPR_LIST:
		case EXPR_TUPLE:
		case EXPR_APP:
			dst->var1 = dst->var2 = NULL;
			if (src->var1) {
				dst->var1 = my_calloc(1, sizeof(struct node));
				cpy_expression_to_node(dst->var1, src->var1);
			}
			if (src->var2) {
				dst->var2 = my_calloc(1, sizeof(struct node));
				cpy_expression_to_node(dst->var2, src->var2);
			}
	}

	dst->used_count = 1;
}

void cpy_node_to_expression(expression* dst, struct node* src) {
	if (!dst || !src)
		return;

	free_expression(dst);
	dst->is_strict = 0;

	dst->kind = src->kind;
	switch (src->kind) {
		case NODE_INT:
			dst->var1 = my_calloc(1, sizeof(int));
			*((int*) dst->var1) = *((int*) src->var1);
			break;

		case NODE_NAME:
			dst->var1 = my_calloc(1, 1 + strlen((char*) src->var1));
			strcpy(dst->var1, src->var1);
			break;

		case NODE_CODE:
			dst->var1 = src->var1;
			dst->var2 = my_calloc(1, sizeof(unsigned char));
			*((unsigned char*) dst->var2) = *((unsigned char*) src->var2);
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			dst->var1 = dst->var2 = NULL;
			if (src->var1) {
				dst->var1 = my_calloc(1, sizeof(expression));
				cpy_node_to_expression(dst->var1, src->var1);
			}
			if (src->var2) {
				dst->var2 = my_calloc(1, sizeof(expression));
				cpy_node_to_expression(dst->var2, src->var2);
			}
			break;
	}
}
