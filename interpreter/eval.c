#include "eval.h"

#include <stdbool.h>
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

typedef struct replacements {
	replacement replacement;
	struct replacements* rest;
} replacements;

void eval(fuspel* rules, struct node** node, bool to_rnf);

void push_repl(replacements* repls, char* name, struct node* node) {
	while (repls->rest) repls = repls->rest;
	repls->rest = my_calloc(1, sizeof(replacements));
	repls->rest->replacement.name = name;
	repls->rest->replacement.node = node;
	repls->rest->rest = NULL;
}

void free_repls(replacements* repls) {
	if (repls) {
		if (repls->rest) {
			free_repls(repls->rest);
			my_free(repls->rest);
			repls->rest = NULL;
		}
		repls->replacement.name = NULL;
		repls->replacement.node = NULL;
	}
}

void replace_all(fuspel *rules, replacements* repls, struct node** node) {
	unsigned int org_used_count;

	if (!node || !*node)
		return;

	switch ((*node)->kind) {
		case NODE_INT:
		case NODE_CODE:
			break;

		case NODE_NAME:
			for (; repls && repls; repls = repls->rest) {
				if (repls->replacement.name && repls->replacement.node &&
						!strcmp(repls->replacement.name,
							(char*) (*node)->var1)) {
					org_used_count = (*node)->used_count;
					if (org_used_count == 1) {
						free_node(*node, 1, 1);
						*node = repls->replacement.node;
						use_node(*node, org_used_count);
					} else {
						free_node(*node, 1, 0);
						(*node)->kind = NODE_REDIRECT;
						(*node)->var1 = repls->replacement.node;
						(*node)->var2 = NULL;
						use_node(*node, org_used_count - 1);
					}
					break;
				}
			}
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			replace_all(rules, repls, (struct node**) &(*node)->var1);
			replace_all(rules, repls, (struct node**) &(*node)->var2);
			break;

		case NODE_REDIRECT:
			replace_all(rules, repls, (struct node**) &(*node)->var1);
			break;
	}
}

bool match_expr(fuspel* rules, expression* expr, struct node** node,
		replacements* repls) {
	replacements* _repls;
	for (_repls = repls; _repls->rest; _repls = _repls->rest);

	remove_redirects(*node);

	if (expr->kind == EXPR_INT ||
			expr->kind == EXPR_LIST ||
			expr->kind == EXPR_TUPLE)
		eval(rules, node, 1);

	switch (expr->kind) {
		case EXPR_INT:
			return *((int*) (*node)->var1) == *((int*) expr->var1);

		case EXPR_NAME:
			push_repl(_repls, (char*) expr->var1, *node);
			return 1;

		case EXPR_LIST:
		case EXPR_TUPLE:
			if ((int) (*node)->kind != (int) expr->kind)
				return 0;

			if (!expr->var1)
				return (*node)->var1 == NULL;

			return
				match_expr(rules, expr->var1, (struct node**) &(*node)->var1, _repls) &&
				match_expr(rules, expr->var2, (struct node**) &(*node)->var2, _repls);

		default:
			return 0;
	}
}

int match_rule(fuspel* rules, rewrite_rule* rule, struct node** node,
		replacements* repls) {
	struct node*** node_args;
	unsigned char i;

	switch ((*node)->kind) {
		case NODE_NAME:
			return (strcmp(rule->name, (char*) (*node)->var1)) ? -1 : 0;
			break;

		case NODE_APP:
			node_args = flatten_app_args(node, true);
			i = 0;
			if (!strcmp((*node_args[0])->var1, rule->name)) {
				struct node** node = node_args[++i];
				arg_list* args = rule->args;
				unsigned char args_len = len_arg_list(args);

				while (!empty_args_list(args)) {
					if (!match_expr(rules, &args->elem, node, repls)) {
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

		case NODE_REDIRECT:
			return match_rule(rules, rule, (*node)->var1, repls);
			break;

		default:
			return -1;
	}
}

bool is_code_app(struct node* node) {
	for (; node->kind == NODE_APP; node = node->var1);
	return node->kind == NODE_CODE;
}

void eval_code_app(fuspel* rules, struct node** node) {
	struct node *root, ***args;
	Code_1* f1;
	Code_2* f2;
	unsigned char i;

	for (root = *node; root->kind == NODE_APP; root = root->var1);
	if (root->kind != NODE_CODE || !root->var1)
		return;

	args = flatten_app_args(node, true);

	for (i = 1; i <= *((unsigned char*) root->var2); i++)
		eval(rules, args[i], 0);

	switch (*((unsigned char*) root->var2)) {
		case 1:
			f1 = (Code_1*) root->var1;
			f1(node, *args[1]);
		case 2:
			f2 = (Code_2*) root->var1;
			f2(node, *args[1], *args[2]);
	}

	my_free(args);
}

#ifdef _FUSPEL_DEBUG
struct node** root_node;
#endif

void eval(fuspel* rules, struct node** node, bool to_rnf) {
	fuspel* _rules;
	bool rerun;
#ifdef _FUSPEL_DEBUG
	bool print_graph;
#endif
	replacements* repls;

	if (!node || !*node)
		return;

	repls = my_calloc(1, sizeof(replacements));

#ifdef _FUSPEL_DEBUG
	if (!root_node) {
		root_node = node;
		print_node_to_file(*root_node, NULL, NULL);
	}
#endif

	do {
		rerun = 0;
#ifdef _FUSPEL_DEBUG
		print_graph = true;
#endif

		switch ((*node)->kind) {
			case NODE_INT:
				break;

			case NODE_NAME:
			case NODE_APP:
				if (is_code_app(*node)) {
					eval_code_app(rules, node);
					rerun = 1;
					break;
				}

				_rules = rules;
				while (_rules) {
					int add_args = match_rule(
							rules, &_rules->rule, node, repls);

					if (add_args >= 0) {
						unsigned char j;
						unsigned int org_used_count;
						replacements* _repls;
						struct node** _node = node;
						struct node* new_node = my_calloc(1, sizeof(struct node));

						for (j = 0; j < add_args; j++)
							_node = (struct node**) &(*_node)->var1;

						org_used_count = (*_node)->used_count;

						for (_repls = repls->rest;
								_repls && _repls->replacement.node;
								_repls = _repls->rest)
							use_node(_repls->replacement.node, 1);

						cpy_expression_to_node(new_node, &_rules->rule.rhs);

						replace_all(rules, repls->rest, &new_node);

						if (org_used_count == 1) {
							free_node(*_node, 1, 1);
							use_node(new_node, org_used_count - 1);
							*_node = new_node;
						} else {
							free_node(*_node, org_used_count, 0);
							(*_node)->kind = NODE_REDIRECT;
							(*_node)->var1 = new_node;
							(*_node)->var2 = NULL;
							(*_node)->used_count = org_used_count;
							use_node(new_node, org_used_count - 1);
						}

						for (_repls = repls->rest;
								_repls && _repls->replacement.node;
								_repls = _repls->rest)
							free_node(_repls->replacement.node, 1, 1);

						rerun = 1;
#ifdef _FUSPEL_DEBUG
						if (is_code_app(*node))
							print_graph = false;
#endif
						break;
					}

					free_repls(repls);

					_rules = _rules->rest;
				}

				free_repls(repls);
				break;

			case NODE_LIST:
				if (!(*node)->var1)
					break;
			case NODE_TUPLE:
				if (to_rnf)
					break;

				eval(rules, (struct node**) &(*node)->var1, to_rnf);
				eval(rules, (struct node**) &(*node)->var2, to_rnf);
				break;

			case NODE_CODE:
				if (*((unsigned char*) (*node)->var2) == 0) {
					Code_0* code_fun = (Code_0*) (*node)->var1;
					code_fun(node);
					use_node(*node, 1);
					rerun = 1;
				}
				break;

			case NODE_REDIRECT:
				remove_redirects(*node);
				rerun = 1;
				break;
		}

#ifdef _FUSPEL_DEBUG
		if (rerun && print_graph)
			print_node_to_file(*root_node, NULL, NULL);
#endif
	} while (rerun);

	free_repls(repls);
	my_free(repls);
}

expression* eval_main(fuspel* rules) {
	struct node* main_node = my_calloc(1, sizeof(struct node));
	expression* expr = my_calloc(1, sizeof(expression));

	main_node->kind = EXPR_NAME;
	main_node->used_count = 1;
	main_node->var1 = my_calloc(1, 5);
	strcpy(main_node->var1, "main");

	eval(rules, &main_node, 0);

	cpy_node_to_expression(expr, main_node);

	free_node(main_node, 1, 1);

	return expr;
}
