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

void eval(fuspel* rules, struct node** node,
		replacements** repls, nodes_array** to_free);

#define eval_rnf(rs, n, repls, tf) eval(rs, n, repls, tf)

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

void free_node(struct node* node, unsigned dont_free_first) {
	if (!node)
		return;

	node->used_count--;

	if (node->kind == EXPR_LIST ||
			node->kind == EXPR_TUPLE ||
			node->kind == EXPR_APP) {
		free_node((struct node*) node->var1, 0);
		free_node((struct node*) node->var2, 0);
	}

	if (node->used_count == 0) {
		if (node->kind == EXPR_INT || node->kind == EXPR_NAME)
			my_free(node->var1);

		if (!dont_free_first)
			my_free(node);
	}
}

struct node** flatten_app_args(struct node* from) {
	struct node** result;
	unsigned int i;
	unsigned char len = 0;
	struct node* _from = from;

	while (_from->kind == EXPR_APP) {
		len++;
		_from = _from->var1;
	}
	len++;

	result = my_calloc(1, sizeof(struct node*) * (len + 1));
	i = 1;
	while (from->kind == EXPR_APP) {
		result[len - i] = from->var2;
		from = from->var1;
		i++;
	}
	result[0] = from;
	result[len] = NULL;
	return result;
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

	dst->used_count = 1;
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
			eval_rnf(rules, node, repls, to_free);

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
	struct node** node_args;
	unsigned char i;

	switch (node->kind) {
		case EXPR_NAME:
			return (strcmp(rule->name, (char*) node->var1)) ? -1 : 0;
			break;

		case EXPR_APP:
			node_args = flatten_app_args(node);
			i = 0;
			if (!strcmp(node_args[0]->var1, rule->name)) {
				struct node* node = node_args[++i];
				arg_list* args = rule->args;
				unsigned char args_len = len_arg_list(args);

				while (!empty_args_list(args)) {
					if (!match_expr(rules, &args->elem, &node, repls, to_free)) {
						my_free(node_args);
						return -1;
					}

					args = args->rest;
					node = node_args[++i];

					if (!empty_args_list(args) && !node) {
						my_free(node_args);
						return -1;
					}
				}

				while (node) node = node_args[++i];
				my_free(node_args);
				return i - args_len - 1;
			}
			my_free(node_args);
			return -1;
			break;

		default:
			return -1;
	}
}

void eval(fuspel* rules, struct node** node,
		replacements** repls, nodes_array** to_free) {
	fuspel* _rules;
	unsigned rerun;

	if (!*repls) {
		*repls = my_calloc(1, sizeof(replacements) + 10 * sizeof(replacement));
		(*repls)->length = 10;
	}

	if (!*to_free) {
		*to_free = my_calloc(1, sizeof(nodes_array) + 10 * sizeof(struct node*));
		(*to_free)->length = 10;
	}

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
							rules, &_rules->rule, *node, repls, to_free);

					if (add_args == 0) {
						free_node(*node, 1);
						cpy_expression_to_node(*node, &_rules->rule.rhs);
						rerun = 1;
						break;
					}
					// TODO add args

					(*to_free)->nodes[0] = NULL;
					(*repls)->replacements[0].name = NULL;
					(*repls)->replacements[0].node = NULL;

					_rules = _rules->rest;
				}
				break;

			case EXPR_LIST:
			case EXPR_TUPLE:
				eval(rules, (struct node**) &(*node)->var1, repls, to_free);
				eval(rules, (struct node**) &(*node)->var2, repls, to_free);
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
	replacements** repls = my_calloc(1, sizeof(replacements*));
	nodes_array** to_free = my_calloc(1, sizeof(nodes_array*));

	main_node->kind = EXPR_NAME;
	main_node->used_count = 1;
	main_node->var1 = my_calloc(1, 5);
	strcpy(main_node->var1, "main");

	eval(rules, &main_node, repls, to_free);

	cpy_node_to_expression(expr, main_node);

	free_node(main_node, 0);

	my_free(*repls);
	my_free(*to_free);
	my_free(repls);
	my_free(to_free);

	return expr;
}
