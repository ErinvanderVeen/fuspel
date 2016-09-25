#include "print.h"

#ifdef _FUSPEL_DEBUG
#include <inttypes.h>
#include <stdbool.h>
#endif

#include "log.h"
#include "mem.h"

void print_token(token* tk) {
	char c = NULL;
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

void print_token_list(token_list* list) {
	print_token(&list->elem);
	if (list->rest) {
		printf(list->elem.kind == TOKEN_SEMICOLON ? "\n" : " ");
		print_token_list(list->rest);
	}
}

void print_in_list(expression* expr) {
	if (!expr->var1)
		return;

	print_expression(expr->var1);

	if (((expression*) expr->var2)->kind == EXPR_LIST) {
		if (((expression*) expr->var2)->var1) {
			printf(",");
			print_in_list(expr->var2);
		}
	} else {
		printf(":");
		print_expression(expr->var2);
	}
}

void print_expression(expression* expr) {
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
			if (((expression*) expr->var1)->kind == EXPR_APP)
				printf("(");
			print_expression(expr->var1);
			if (((expression*) expr->var1)->kind == EXPR_APP)
				printf(")");
			printf(" ");
			if (((expression*) expr->var2)->kind == EXPR_APP)
				printf("(");
			print_expression(expr->var2);
			if (((expression*) expr->var2)->kind == EXPR_APP)
				printf(")");
			break;
	}
}

void print_arg_list(arg_list* args) {
	if (!args)
		return;

	print_expression(&args->elem);
	printf(" ");
	if (args->rest)
		print_arg_list(args->rest);
}

void print_rewrite_rule(rewrite_rule* rule) {
	printf("%s ", rule->name);
	print_arg_list(rule->args);
	printf("= ");
	print_expression(&rule->rhs);
	printf(";");
}

void print_fuspel(fuspel* rules) {
	print_rewrite_rule(&rules->rule);
	if (rules->rest) {
		printf("\n");
		print_fuspel(rules->rest);
	}
}

void print_node(struct node* node) {
	expression* e = my_calloc(1, sizeof(expression));
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

void print_node_to_file(struct node* node, FILE* f, struct visited_nodes *visited) {
	bool close = 0;
	bool do_free_visited = 0;
	unsigned int borderwidth, edgewidth;

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
		fprintf(f, "node [shape=rectangle];\n");
		close = 1;
	}

	borderwidth = node->used_count > 20 ? 20 : node->used_count;
	edgewidth = borderwidth > 5 ? 5 : borderwidth;

	switch (node->kind) {
		case NODE_INT:
			fprintf(f, "%" PRIuPTR " [label=\"%d (%d)\", penwidth=%d];\n",
					(uintptr_t) node, *((int*) node->var1), node->used_count, borderwidth);
			break;

		case NODE_NAME:
			fprintf(f, "%" PRIuPTR " [label=\"%s (%d)\", penwidth=%d];\n",
					(uintptr_t) node, (char*) node->var1, node->used_count, borderwidth);
			break;

		case NODE_CODE:
			fprintf(f, "%" PRIuPTR " [label=\"code: %p (%d)\", penwidth=%d];\n",
					(uintptr_t) node, node->var1, node->used_count, borderwidth);
			break;

		case NODE_LIST:
		case NODE_TUPLE:
		case NODE_APP:
			if (node->kind == NODE_LIST)
				fprintf(f, "%" PRIuPTR " [label=\"List (%d)\", color=gray, penwidth=%d];\n",
						(uintptr_t) node, node->used_count, borderwidth);
			else if (node->kind == NODE_TUPLE)
				fprintf(f, "%" PRIuPTR " [label=\"Tuple (%d)\", color=gray, penwidth=%d];\n",
						(uintptr_t) node, node->used_count, borderwidth);
			else if (node->kind == NODE_APP)
				fprintf(f, "%" PRIuPTR " [label=\"App (%d)\", color=gray, penwidth=%d];\n",
						(uintptr_t) node, node->used_count, borderwidth);

			if (node->var1) {
				print_node_to_file((struct node*) node->var1, f, visited);
				print_node_to_file((struct node*) node->var2, f, visited);
				fprintf(f, "%" PRIuPTR " -> %" PRIuPTR " [label=\"l\", penwidth=%d];\n",
						(uintptr_t) node, (uintptr_t) node->var1, edgewidth);
				fprintf(f, "%" PRIuPTR " -> %" PRIuPTR " [label=\"r\", penwidth=%d];\n",
						(uintptr_t) node, (uintptr_t) node->var2, edgewidth);
			}
			break;

		case NODE_REDIRECT:
			fprintf(f, "%" PRIuPTR " [label=\"Redirection (%d)\", color=gray, penwidth=%d];\n",
					(uintptr_t) node, node->used_count, borderwidth);
			print_node_to_file((struct node*) node->var1, f, visited);
			fprintf(f, "%" PRIuPTR " -> %" PRIuPTR " [penwidth=%d];\n",
					(uintptr_t) node, (uintptr_t) node->var1, edgewidth);
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
