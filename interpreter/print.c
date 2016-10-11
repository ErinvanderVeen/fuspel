#include "print.h"

#ifdef _FUSPEL_DEBUG
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "code.h"
#endif

#include "mem.h"

void print_token(struct token *tk) {
	char c = 0x00;
	switch (tk->kind) {
		case TOKEN_SEMICOLON: c = ';'; break;
		case TOKEN_COLON:     c = ':'; break;
		case TOKEN_OPEN_P:    c = '('; break;
		case TOKEN_CLOSE_P:   c = ')'; break;
		case TOKEN_OPEN_SQ:   c = '['; break;
		case TOKEN_CLOSE_SQ:  c = ']'; break;
		case TOKEN_EQUALS:    c = '='; break;
		case TOKEN_COMMA:     c = ','; break;
		case TOKEN_CODE:      printf("code "); return;
		case TOKEN_IMPORT:    printf("import "); return;
		case TOKEN_NAME:
			printf("%s", (char*) tk->var);
			return;
		case TOKEN_INT:
			printf("%d", *((int*) tk->var));
			return;
	}
	if (c)
		printf("%c", c);
}

void print_token_list(struct token_list *list) {
	print_token(&list->elem);
	if (list->rest) {
		printf(list->elem.kind == TOKEN_SEMICOLON ? "\n" : " ");
		print_token_list(list->rest);
	}
}

void print_in_list(struct expression *expr) {
	if (!expr->var1)
		return;

	print_expression(expr->var1);

	if (((struct expression*) expr->var2)->kind == EXPR_LIST) {
		if (((struct expression*) expr->var2)->var1) {
			printf(",");
			print_in_list(expr->var2);
		}
	} else {
		printf(":");
		print_expression(expr->var2);
	}
}

void print_expression(struct expression *expr) {
	if (!expr)
		return;

	switch (expr->kind) {
		case EXPR_INT:
			printf("%d", *((int*) expr->var1));
			break;
		case EXPR_NAME:
			printf("%s", (char*) expr->var1);
			break;
		case EXPR_CODE:
			printf("code [%p, %d args]",
					(void*) expr->var1, *((unsigned char*) expr->var2));
			break;
		case EXPR_LIST:
			printf("[");
			print_in_list(expr);
			printf("]");
			break;
		case EXPR_TUPLE:
			printf("(");
			print_expression(expr->var1);
			printf(",");
			print_expression(expr->var2);
			printf(")");
			break;
		case EXPR_APP:
			if (((struct expression*) expr->var1)->kind == EXPR_APP)
				printf("(");
			print_expression(expr->var1);
			if (((struct expression*) expr->var1)->kind == EXPR_APP)
				printf(")");
			printf(" ");
			if (((struct expression*) expr->var2)->kind == EXPR_APP)
				printf("(");
			print_expression(expr->var2);
			if (((struct expression*) expr->var2)->kind == EXPR_APP)
				printf(")");
			break;
	}
}

void print_arg_list(struct arg_list *args) {
	if (!args)
		return;

	print_expression(&args->elem);
	printf(" ");
	if (args->rest)
		print_arg_list(args->rest);
}

void print_rewrite_rule(struct rewrite_rule *rule) {
	printf("%s ", rule->name);
	print_arg_list(rule->args);
	printf("= ");
	print_expression(&rule->rhs);
	printf(";");
}

void print_fuspel(struct fuspel *rules) {
	print_rewrite_rule(&rules->rule);
	if (rules->rest) {
		printf("\n");
		print_fuspel(rules->rest);
	}
}

void print_node(struct node *node) {
	struct expression *e = my_calloc(1, sizeof(struct expression));
	cpy_node_to_expression(e, node);
	print_expression(e);
	free_expression(e);
	my_free(e);
}

#ifdef _FUSPEL_DEBUG
static unsigned int file_count = 0;

void free_visited_nodes(struct visited_nodes *list) {
	if (list) {
		free_visited_nodes(list->next);
		my_free(list);
	}
}

void push_visited_node(struct visited_nodes *list, struct node *node) {
	while (list->next) list = list->next;
	list->next = my_calloc(1, sizeof(struct visited_nodes));
	list->next->node = node;
}

bool visited_node_exists(struct visited_nodes *list, struct node *node) {
	while (list) {
		if (list->node == node)
			return 1;
		list = list->next;
	}
	return 0;
}

char *get_app_name(struct node *node) {
	char *name;
	if (node->kind == NODE_NAME) {
		name = my_calloc(strlen(node->var1) + 1, 1);
		strcpy(name, (char*) node->var1);
	} else if (node->kind == NODE_CODE) {
		name = my_calloc(24, 1);
		strncpy(name, code_find_name(node->var1), 23);
	} else {
		name = my_calloc(9, 1);
		strcpy(name, "Redirect");
	}
	return name;
}

void print_node_to_file(struct node *node, FILE *f, struct visited_nodes *visited) {
	bool close = 0;
	bool do_free_visited = 0;

	struct node ***args_list;
	unsigned int arg_i;
	char *app_name;

	if (visited_node_exists(visited, node))
		return;

	if (!visited) {
		visited = my_calloc(1, sizeof(struct visited_nodes));
		do_free_visited = 1;
	}

	push_visited_node(visited, node);

	if (!f) {
		char fname[20];
		sprintf(fname, "graph-%03u.dot", file_count++);
		f = fopen(fname, "w");
		fprintf(f, "digraph {\n");
		fprintf(f, "node [shape=record];\n");
		close = 1;
	}

	switch (node->kind) {
		case NODE_INT:
			fprintf(f, "%" PRIuPTR " [shape=diamond,label=\"%d\"];\n",
					(uintptr_t) node, *((int*) node->var1));
			break;

		case NODE_NAME:
			fprintf(f, "%" PRIuPTR " [label=\"%s\"];\n",
					(uintptr_t) node, (char*) node->var1);
			break;
		case NODE_CODE:
			fprintf(f, "%" PRIuPTR " [label=\"%s\"];\n",
					(uintptr_t) node, code_find_name(node->var1));
			break;

		case NODE_APP:
			args_list = flatten_app_args(&node, false);
			app_name = get_app_name(*args_list[0]);
			fprintf(f, "%" PRIuPTR " [label=\"%s", (uintptr_t) node, app_name);
			my_free(app_name);
			for (arg_i = 1; args_list[arg_i]; arg_i++)
				fprintf(f, "|<%u>", arg_i);
			fprintf(f, "\"];\n");
			for (arg_i = 1; args_list[arg_i]; arg_i++) {
				print_node_to_file(*args_list[arg_i], f, visited);
				fprintf(f, "%" PRIuPTR ":%d -> %" PRIuPTR ";\n",
						(uintptr_t) node,
						arg_i,
						(uintptr_t) *args_list[arg_i]);
			}
			my_free(args_list);
			break;

		case NODE_LIST:
			if (!node->var1) {
				fprintf(f, "%" PRIuPTR " [label=\"Nil\"];\n",
						(uintptr_t) node);
				break;
			}
		case NODE_TUPLE:
			if (node->kind == NODE_LIST)
				fprintf(f, "%" PRIuPTR " [label=\"Cons|<l>|<r>\", color=gray, fontcolor=gray];\n",
						(uintptr_t) node);
			else if (node->kind == NODE_TUPLE)
				fprintf(f, "%" PRIuPTR " [label=\"Tuple|<l>|<r>\", color=gray, fontcolor=gray];\n",
						(uintptr_t) node);

			print_node_to_file((struct node*) node->var1, f, visited);
			print_node_to_file((struct node*) node->var2, f, visited);
			fprintf(f, "%" PRIuPTR ":l -> %" PRIuPTR ";\n",
					(uintptr_t) node, (uintptr_t) node->var1);
			fprintf(f, "%" PRIuPTR ":r -> %" PRIuPTR ";\n",
					(uintptr_t) node, (uintptr_t) node->var2);
			break;

		case NODE_REDIRECT:
			fprintf(f, "%" PRIuPTR " [label=\"Redirect\", color=gray, fontcolor=gray];\n",
					(uintptr_t) node);
			print_node_to_file((struct node*) node->var1, f, visited);
			fprintf(f, "%" PRIuPTR " -> %" PRIuPTR ";\n",
					(uintptr_t) node, (uintptr_t) node->var1);
			break;
	}

	if (close) {
		fprintf(f, "}");
		fclose(f);
	}

	if (do_free_visited) {
		free_visited_nodes(visited);
	}
}
#endif
