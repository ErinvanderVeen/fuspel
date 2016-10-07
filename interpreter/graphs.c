#include "graphs.h"

#include <string.h>

#include "mem.h"

void use_node(struct node *node, unsigned int count) {
	if (!node)
		return;

	node->used_count += count;

	switch (node->kind) {
		case NODE_INT:
		case NODE_NAME:
		case NODE_CODE:
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			use_node(node->var2, count);

		case NODE_REDIRECT:
			use_node(node->var1, count);
			break;
	}
}

void free_node(struct node *node, unsigned int count, bool free_first) {
	if (!node)
		return;

	node->used_count -= count;

	switch (node->kind) {
		case NODE_INT:
		case NODE_NAME:
		case NODE_CODE:
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			free_node(node->var2, count, 1);

		case NODE_REDIRECT:
			free_node(node->var1, count, 1);
			break;
	}

	if (node->used_count == 0) {
		if (node->kind == NODE_INT || node->kind == NODE_NAME)
			my_free(node->var1);

		if (node->kind == NODE_CODE)
			my_free(node->var2);

		node->var1 = node->var2 = NULL;

		if (free_first)
			my_free(node);
	}
}

struct node ***flatten_app_args(struct node **from, bool remove_redirs) {
	struct node ***result;
	unsigned int i;
	unsigned char len = 0;
	struct node *_from = *from;

	if (remove_redirs)
		remove_redirects(_from);

	while (_from->kind == NODE_APP) {
		len++;
		_from = _from->var1;
		if (remove_redirs)
			remove_redirects(_from);
	}
	len++;

	result = my_calloc(1, sizeof(struct node**) * (len + 1));
	i = 1;
	while ((*from)->kind == NODE_APP) {
		result[len - i] = (struct node**) &(*from)->var2;
		from = (struct node**) &(*from)->var1;
		i++;
	}
	result[0] = from;
	result[len] = NULL;
	return result;
}

void remove_redirects(struct node *node) {
	while (node->kind == NODE_REDIRECT) {
		struct node *child = (struct node*) node->var1;
		node->kind = child->kind;
		node->var1 = child->var1;
		node->var2 = child->var2;
		child->used_count -= node->used_count;
		if (child->used_count == 0)
			my_free(child);
	}
}

void cpy_expression_to_node(struct node *dst, struct expression *src) {
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

void cpy_node_to_expression(struct expression *dst, struct node *src) {
	if (!dst || !src)
		return;

	free_expression(dst);

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
				dst->var1 = my_calloc(1, sizeof(struct expression));
				cpy_node_to_expression(dst->var1, src->var1);
			}
			if (src->var2) {
				dst->var2 = my_calloc(1, sizeof(struct expression));
				cpy_node_to_expression(dst->var2, src->var2);
			}
			break;

		case NODE_REDIRECT:
			cpy_node_to_expression(dst, src->var1);
			break;
	}
}
