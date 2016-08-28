#include "eval.h"

#include <string.h>

#include "code.h"
#include "mem.h"

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

typedef struct {
	char* name;
	struct node* node;
} replacement;

typedef struct {
	unsigned int length;
	replacement replacements[1];
} replacements;

void eval(fuspel*, struct node**);

#define eval_rnf(rs, n) eval(rs, n)

nodes_array* push_node(nodes_array* nodes, struct node* node) {
	unsigned int i;
	for (i = 0; i < nodes->length && nodes->nodes[i]; i++);
	if (nodes->nodes[i])
		nodes = my_realloc(nodes, sizeof(nodes_array) +
				2 * nodes->length * sizeof(*node));
	nodes->nodes[i] = node;
	return nodes;
}

replacements* push_repl(replacements* repls, char* name, struct node* node) {
	unsigned int i;
	for (i = 0; i < repls->length && repls->replacements[i].name; i++);
	if (repls->replacements[i].name)
		repls = my_realloc(repls, sizeof(replacements) +
				2 * repls->length * sizeof(replacement));
	repls->replacements[i].name = name;
	repls->replacements[i].node = node;
	return repls;
}

void free_node(struct node* node) {
	if (!node)
		return;

	free_node(node->var1);
	free_node(node->var2);
	my_free(node->var1);
	my_free(node->var2);
}

void cpy_expression_to_node(struct node* dst, expression* src) {
	if (!dst || !src)
		return;

	free_node(dst);

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

		default:
			if (src->var1) {
				dst->var1 = my_calloc(1, sizeof(struct node));
				cpy_expression_to_node(dst->var1, src->var1);
			}
			if (src->var2) {
				dst->var2 = my_calloc(1, sizeof(struct node));
				cpy_expression_to_node(dst->var2, src->var2);
			}
	}
}

void cpy_node_to_expression(expression* dst, struct node* src) {
	if (!dst || !src)
		return;

	free_expression(dst);

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

		default:
			if (src->var1) {
				dst->var1 = my_calloc(1, sizeof(expression));
				cpy_node_to_expression(dst->var1, src->var1);
			}
			if (src->var2) {
				dst->var2 = my_calloc(1, sizeof(expression));
				cpy_node_to_expression(dst->var2, src->var2);
			}
	}
}

unsigned match_expr(fuspel* rules, expression* expr, struct node** node,
		replacements** repls, nodes_array** to_free) {

	switch (expr->kind) {
		case EXPR_INT:
			return *((int*) (*node)->var1) == *((int*) expr->var1);

		case EXPR_NAME:
			*repls = push_repl(*repls, (char*) expr->var1, *node);
			return 1;

		case EXPR_LIST:
		case EXPR_TUPLE:
			eval_rnf(rules, node);

			if ((*node)->kind != expr->kind)
				return 0;

			*to_free = push_node(*to_free, *node);

			if (expr->var1 == NULL)
				return (*node)->var1 == NULL;

			return
				match_expr(rules, expr->var1, (*node)->var1, repls, to_free) &&
				match_expr(rules, expr->var2, (*node)->var2, repls, to_free);

		default:
			return 0;
	}
}

int match_rule(fuspel* rules, rewrite_rule* rule, struct node* node,
		replacements** repls, nodes_array** to_free) {
	switch (node->kind) {
		case EXPR_NAME:
			return (strcmp(rule->name, (char*) node->var1)) ? -1 : 0;
			break;

		default:
			return -1;
	}
}

void eval(fuspel* rules, struct node** node) {
	fuspel* _rules;
	replacements* repls;
	nodes_array* to_free;
	unsigned rerun;

	repls = my_calloc(1, sizeof(replacements) + 10 * sizeof(replacement));
	repls->length = 10;

	to_free = my_calloc(1, sizeof(nodes_array) + 10 * sizeof(struct node*));
	to_free->length = 10;

	do {
		rerun = 0;

		switch ((*node)->kind) {
			case EXPR_INT:
				break;

			case EXPR_NAME:
			case EXPR_APP:
				_rules = rules;
				while (_rules) {
					int add_args = match_rule(
							rules, &_rules->rule, *node, &repls, &to_free);

					if (add_args == 0) {
						cpy_expression_to_node(*node, &_rules->rule.rhs);
						rerun = 1;
						break;
					}
					// TODO add args

					to_free->nodes[0] = NULL;
					repls->replacements[0].name = NULL;
					repls->replacements[0].node = NULL;

					_rules = _rules->rest;
				}
				break;

			case EXPR_LIST:
			case EXPR_TUPLE:
				eval(rules, (struct node**) &(*node)->var1);
				eval(rules, (struct node**) &(*node)->var2);
				break;

			default:
				//TODO
				break;
		}
	} while (rerun);
}

expression* eval_main(fuspel* rules) {
	struct node* main_node = my_calloc(1, sizeof(struct node));
	expression* expr = my_calloc(1, sizeof(expression));

	main_node->kind = EXPR_NAME;
	main_node->var1 = my_calloc(1, 5);
	strcpy(main_node->var1, "main");

	eval(rules, &main_node);

	cpy_node_to_expression(expr, main_node);

	return expr;
}
