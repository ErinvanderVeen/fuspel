#include "eval.h"

#include <string.h>

#include "code.h"
#include "graphs.h"
#include "mem.h"

#include "print.h"

typedef struct {
	char* name;
	struct node* node;
} replacement;

typedef struct replacements {
	replacement replacement;
	struct replacements* rest;
} replacements;

void eval(fuspel* rules, struct node** node, replacements* repls,
		unsigned to_rnf);

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

void replace_all(replacements* repls, struct node** node) {
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
					printf("Replacing %s with %p on %p / %p\n",
							(char*) (*node)->var1,
							repls->replacement.node,
							node,
							*node);
					org_used_count = (*node)->used_count;
					free_node(*node, 1, 1);
					*node = repls->replacement.node;
					use_node(*node, org_used_count);
					break;
				}
			}
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			replace_all(repls, (struct node**) &(*node)->var1);
			replace_all(repls, (struct node**) &(*node)->var2);
			break;
	}
}

struct node*** flatten_app_args(struct node** from) {
	struct node ***result;
	unsigned int i;
	unsigned char len = 0;
	struct node* _from = *from;

	while (_from->kind == NODE_APP) {
		len++;
		_from = _from->var1;
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

unsigned match_expr(fuspel* rules, expression* expr, struct node** node,
		replacements* repls) {
	replacements* _repls;
	for (_repls = repls; _repls->rest; _repls = _repls->rest);

	if (expr->kind == EXPR_INT ||
			expr->kind == EXPR_LIST ||
			expr->kind == EXPR_TUPLE)
		eval(rules, node, NULL, 1);

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
			node_args = flatten_app_args(node);
			i = 0;
			if (!strcmp((*node_args[0])->var1, rule->name)) {
				struct node** node = node_args[++i];
				arg_list* args = rule->args;
				unsigned char args_len = len_arg_list(args);

				printf("RULE: %s\n", rule->name);

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

		default:
			return -1;
	}
}

unsigned is_code_app(struct node* node) {
	for (; node->kind == NODE_APP; node = node->var1);
	return node->kind == NODE_CODE;
}

void eval_code_app(fuspel* rules, struct node** node, replacements* repls) {
	struct node *root, ***args;
	Code_1* f1;
	Code_2* f2;
	unsigned char i;

	for (root = *node; root->kind == NODE_APP; root = root->var1);
	if (root->kind != NODE_CODE || !root->var1)
		return;

	args = flatten_app_args(node);

	for (i = 1; i <= *((unsigned char*) root->var2); i++)
		eval(rules, args[i], repls, 0);

	switch (*((unsigned char*) root->var2)) {
		case 1:
			f1 = (Code_1*) root->var1;
			f1(node, *args[1]);
		case 2:
			f2 = (Code_2*) root->var1;
			f2(node, *args[1], *args[2]);
	}

	use_node(*node, 1);

	my_free(args);
}

#ifdef _FUSPEL_DEBUG
struct node** root_node;
#endif

void eval(fuspel* rules, struct node** node, replacements* repls,
		unsigned to_rnf) {
	fuspel* _rules;
	unsigned rerun, do_free_repls = 0;

	if (!node || !*node)
		return;

	if (!repls) {
		repls = my_calloc(1, sizeof(replacements));
		do_free_repls = 1;
	}

#ifdef _FUSPEL_DEBUG
	if (!root_node) {
		root_node = node;
		print_node_to_file(*root_node, NULL, NULL);
	}
#endif

	do {
		replacements* _repls;

		rerun = 0;

		printf("\nREWRITING %p / %p: ", node, *node);
		print_node(*node);
		printf("... (repls %p)\n", repls);

		switch ((*node)->kind) {
			case NODE_INT:
				break;

			case NODE_NAME:
			case NODE_APP:
				if (is_code_app(*node)) {
					eval_code_app(rules, node, repls);
					rerun = 1;
					break;
				}

				for (_repls = repls; _repls->rest; _repls = _repls->rest);

				_rules = rules;
				while (_rules) {
					int add_args = match_rule(
							rules, &_rules->rule, node, _repls);

					if (add_args >= 0) {
						unsigned char j;
						unsigned int org_used_count;
						replacements* __repls;
						struct node** _node = node;
						struct node* new_node = my_calloc(1, sizeof(struct node));

						for (j = 0; j < add_args; j++)
							_node = (struct node**) &(*_node)->var1;

						org_used_count = (*_node)->used_count;

						for (__repls = _repls->rest;
								__repls && __repls->replacement.node;
								__repls = __repls->rest)
							use_node(__repls->replacement.node, 1);

						printf("Replacing <");
						print_node(*_node);
						printf(">: <");
						cpy_expression_to_node(new_node, &_rules->rule.rhs);
						print_node(new_node);
						printf(">\n");

						replace_all(_repls->rest, &new_node);
						use_node(new_node, org_used_count - 1);

						for (__repls = _repls->rest;
								__repls && __repls->replacement.node;
								__repls = __repls->rest)
							free_node(__repls->replacement.node, 1, 1);

						free_node(*_node, 1, 1);
						*_node = new_node;

						rerun = 1;
						break;
					}

					free_repls(_repls->rest);

					_rules = _rules->rest;
				}

				free_repls(_repls);
				break;

			case NODE_LIST:
				if (!(*node)->var1)
					break;
			case NODE_TUPLE:
				if (to_rnf)
					break;

				eval(rules, (struct node**) &(*node)->var1, repls, to_rnf);
				eval(rules, (struct node**) &(*node)->var2, repls, to_rnf);
				break;

			case NODE_CODE:
				//TODO
				break;
		}

#ifdef _FUSPEL_DEBUG
		if (rerun)
			print_node_to_file(*root_node, NULL, NULL);
#endif
	} while (rerun);

	if (do_free_repls) {
		free_repls(repls);
		my_free(repls);
	}
}

expression* eval_main(fuspel* rules) {
	struct node* main_node = my_calloc(1, sizeof(struct node));
	expression* expr = my_calloc(1, sizeof(expression));
	replacements* repls = my_calloc(1, sizeof(replacements));

	main_node->kind = EXPR_NAME;
	main_node->used_count = 1;
	main_node->var1 = my_calloc(1, 5);
	strcpy(main_node->var1, "main");

	eval(rules, &main_node, repls, 0);

	cpy_node_to_expression(expr, main_node);

	free_node(main_node, 1, 1);

	free_repls(repls);
	my_free(repls);

	return expr;
}
