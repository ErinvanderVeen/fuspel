#include "eval.h"

#include <string.h>

#include "code.h"
#include "graphs.h"
#include "mem.h"

#ifdef _FUSPEL_DEBUG
#include "print.h"
#endif

typedef struct {
	char* name;
	struct node* node;
} replacement;

typedef struct {
	unsigned int length;
	replacement replacements[1];
} replacements;

void eval(fuspel* rules, struct node** node,
		replacements** repls, nodes_array** to_free, unsigned to_rnf);

replacements* push_repl(replacements* repls, char* name, struct node* node) {
	unsigned int i;
	for (i = 0; i < repls->length && repls->replacements[i].name; i++);
	if (repls->replacements[i].name)
		repls = my_realloc(repls, sizeof(replacements) +
				2 * repls->length * sizeof(replacement));
	repls->replacements[i].name = name;
	repls->replacements[i].node = node;
	repls->replacements[i + 1].name = NULL;
	repls->replacements[i + 1].node = NULL;
	return repls;
}

void replace_all(replacements* repls, struct node** node) {
	unsigned char i;
	unsigned int org_used_count;

	if (!node || !*node)
		return;

	switch ((*node)->kind) {
		case EXPR_INT:
		case EXPR_CODE:
			break;

		case EXPR_NAME:
			for (i = 0; repls->replacements[i].name; i++) {
				if (!strcmp(repls->replacements[i].name,
							(char*) (*node)->var1)) {
					org_used_count = (*node)->used_count;
					free_node(*node, 1, 1);
					*node = repls->replacements[i].node;
					use_node(*node, org_used_count);
					break;
				}
			}
			break;

		case EXPR_LIST:
		case EXPR_TUPLE:
		case EXPR_APP:
			replace_all(repls, (struct node**) &(*node)->var1);
			replace_all(repls, (struct node**) &(*node)->var2);
			break;
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

unsigned match_expr(fuspel* rules, expression* expr, struct node** node,
		replacements** repls, nodes_array** to_free) {

	if (expr->kind != EXPR_NAME)
		eval(rules, node, repls, to_free, 1);

	switch (expr->kind) {
		case EXPR_INT:
			return *((int*) (*node)->var1) == *((int*) expr->var1);

		case EXPR_NAME:
			*repls = push_repl(*repls, (char*) expr->var1, *node);
			return 1;

		case EXPR_LIST:
		case EXPR_TUPLE:
			if ((*node)->kind != expr->kind)
				return 0;

			*to_free = push_node(*to_free, *node);

			if (!expr->var1)
				return (*node)->var1 == NULL;

			return
				match_expr(rules, expr->var1, (struct node**) &(*node)->var1, repls, to_free) &&
				match_expr(rules, expr->var2, (struct node**) &(*node)->var2, repls, to_free);

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

unsigned is_code_app(struct node* node) {
	for (; node->kind == EXPR_APP; node = node->var1);
	return node->kind == EXPR_CODE;
}

void eval_code_app(fuspel* rules, struct node** node,
		replacements** repls, nodes_array** to_free) {
	struct node *root, **args;
	Code_1* f1;
	Code_2* f2;
	unsigned char i;

	for (root = *node; root->kind == EXPR_APP; root = root->var1);
	if (root->kind != EXPR_CODE || !root->var1)
		return;

	args = flatten_app_args(*node);

	for (i = 1; i <= *((unsigned char*) root->var2); i++)
		eval(rules, &args[i], repls, to_free, 0);

	switch (*((unsigned char*) root->var2)) {
		case 1:
			f1 = (Code_1*) root->var1;
			f1(node, args[1]);
		case 2:
			f2 = (Code_2*) root->var1;
			f2(node, args[1], args[2]);
	}

	use_node(*node, 1);

	my_free(args);
}

#ifdef _FUSPEL_DEBUG
struct node* root_node;
#endif

void eval(fuspel* rules, struct node** node,
		replacements** repls, nodes_array** to_free, unsigned to_rnf) {
	fuspel* _rules;
	unsigned rerun;

	if (!node || !*node)
		return;

	if (!*repls) {
		*repls = my_calloc(1, sizeof(replacements) + 10 * sizeof(replacement));
		(*repls)->length = 10;
	}

	if (!*to_free) {
		*to_free = my_calloc(1, sizeof(nodes_array) + 10 * sizeof(struct node*));
		(*to_free)->length = 10;
	}

#ifdef _FUSPEL_DEBUG
	if (!root_node) {
		root_node = *node;
		print_node_to_file(root_node, NULL);
	}
#endif

	do {
		rerun = 0;

		switch ((*node)->kind) {
			case EXPR_INT:
				break;

			case EXPR_NAME:
			case EXPR_APP:
				if (is_code_app(*node)) {
					eval_code_app(rules, node, repls, to_free);
					rerun = 1;
					break;
				}

				_rules = rules;
				while (_rules) {
					int add_args = match_rule(
							rules, &_rules->rule, *node, repls, to_free);

					if (add_args >= 0) {
						unsigned char j;
						unsigned int org_used_count;
						struct node** _node = node;

						for (j = 0; j < add_args; j++)
							_node = (struct node**) &(*_node)->var1;

						org_used_count = (*_node)->used_count;

						for (j = 0; (*repls)->replacements[j].node; j++)
							use_node((*repls)->replacements[j].node, 1);

						free_node(*_node, 1, 0);
						cpy_expression_to_node(*_node, &_rules->rule.rhs);
						replace_all(*repls, _node);
						use_node(*_node, org_used_count - 1);

						for (j = 0; (*repls)->replacements[j].node; j++)
							free_node((*repls)->replacements[j].node, add_args + 1, 1);

						(*to_free)->nodes[0] = NULL;
						(*repls)->replacements[0].name = NULL;
						(*repls)->replacements[0].node = NULL;

						rerun = 1;
						break;
					}

					(*to_free)->nodes[0] = NULL;
					(*repls)->replacements[0].name = NULL;
					(*repls)->replacements[0].node = NULL;

					_rules = _rules->rest;
				}
				break;

			case EXPR_LIST:
				if (!(*node)->var1)
					break;
			case EXPR_TUPLE:
				if (to_rnf)
					break;

				eval(rules, (struct node**) &(*node)->var1, repls, to_free, to_rnf);
				eval(rules, (struct node**) &(*node)->var2, repls, to_free, to_rnf);
				break;

			case EXPR_CODE:
				//TODO
				break;
		}

#ifdef _FUSPEL_DEBUG
		if (rerun)
			print_node_to_file(root_node, NULL);
#endif
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

	eval(rules, &main_node, repls, to_free, 0);

	cpy_node_to_expression(expr, main_node);

	printf("main is used %d time(s)\n", main_node->used_count);
	free_node(main_node, 1, 1);

	my_free(*repls);
	my_free(*to_free);
	my_free(repls);
	my_free(to_free);

	return expr;
}
