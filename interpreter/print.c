#include "print.h"

#include <stdio.h>

#include "log.h"

void print_token(token* tk) {
	char c;
	switch (tk->kind) {
		case TOKEN_SEMICOLON: c = ';'; break;
		case TOKEN_COLON:     c = ':'; break;
		case TOKEN_OPEN_P:    c = '('; break;
		case TOKEN_CLOSE_P:   c = ')'; break;
		case TOKEN_OPEN_SQ:   c = '['; break;
		case TOKEN_CLOSE_SQ:  c = ']'; break;
		case TOKEN_EQUALS:    c = '='; break;
		case TOKEN_COMMA:     c = ','; break;
		case TOKEN_STRICT:    c = '!'; break;
		case TOKEN_CODE:      printf("code "); return;
		case TOKEN_IMPORT:    printf("import "); return;
		case TOKEN_NAME:
			printf("%s", (char*) tk->var);
			return;
		case TOKEN_INT:
			printf("%d", *((int*) tk->var));
			return;
	}
	printf("%c", c);
}

void print_token_list(token_list* list) {
	print_token(&list->elem);
	if (list->rest) {
		printf(list->elem.kind == TOKEN_SEMICOLON ? "\n" : " ");
		print_token_list(list->rest);
	}
}

void print_expression(expression* expr) {
	if (!expr)
		return;

	if (expr->is_strict)
		printf("!");

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
			if (!expr->var1) {
				printf("[]");
			} else {
				printf("[");
				print_expression(expr->var1);
				printf(":");
				print_expression(expr->var2);
				printf("]");
			}
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
