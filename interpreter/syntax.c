#include "syntax.h"

#include <string.h>

#include "mem.h"

void free_token(token* tk) {
	if (tk->var)
		my_free(tk->var);
}

void free_token_list(token_list* list) {
	free_token(&list->elem);
	if (list->rest)
		free_token_list(list->rest);
	my_free(list->rest);
}

bool empty_args_list(arg_list* list) {
	return !list;
}

unsigned char len_arg_list(arg_list* list) {
	unsigned char i = 0;
	while (list) {
		i++;
		list = list->rest;
	}
	return i;
}

void cpy_expression(expression* dst, expression* src) {
	free_expression(dst);
	dst->kind = src->kind;
	switch (dst->kind) {
		case EXPR_INT:
			dst->var1 = my_calloc(1, sizeof(int));
			*((int*) dst->var1) = *((int*) src->var1);
			break;
		case EXPR_NAME:
			dst->var1 = my_calloc(1, strlen((char*) src->var1) + 1);
			strcpy(dst->var1, src->var1);
			break;
		case EXPR_CODE:
			dst->var1 = src->var1;
			dst->var2 = my_calloc(1, sizeof(unsigned char));
			*((unsigned char*) dst->var2) = *((unsigned char*) src->var2);
			break;
		case EXPR_LIST:
			if (!src->var1)
				break;
		case EXPR_TUPLE:
		case EXPR_APP:
			dst->var1 = my_calloc(1, sizeof(expression));
			dst->var2 = my_calloc(1, sizeof(expression));
			cpy_expression(dst->var1, src->var1);
			cpy_expression(dst->var2, src->var2);
			break;
	}
}

bool eq_expression(expression* a, expression* b) {
	if (a->kind != b->kind)
		return 0;

	switch (a->kind) {
		case EXPR_INT:  return *((int*) a->var1) == *((int*) b->var1);
		case EXPR_NAME: return !strcmp(a->var1, b->var1);
		case EXPR_CODE: return a->var1 == b->var1;
		case EXPR_TUPLE:
		case EXPR_LIST:
		case EXPR_APP:
			if ((!a->var1 && b->var1) || (a->var1 && !b->var1) ||
					(!a->var2 && b->var2) || (a->var2 && b->var2))
				return 0;
			if (a->var1 && !eq_expression(a->var1, b->var1))
				return 0;
			if (a->var2 && !eq_expression(a->var2, b->var2))
				return 0;
			return 1;
	}

	return 0;
}

void concat_fuspel(fuspel* start, fuspel* end) {
	while (start) {
		if (!start->rest) {
			start->rest = end;
			return;
		}
		start = start->rest;
	}
}

fuspel* push_fuspel(fuspel* rules) {
	fuspel* new_rules = my_calloc(1, sizeof(fuspel));
	new_rules->rest = rules;
	return new_rules;
}

fuspel* pop_fuspel(fuspel* rules) {
	free_rewrite_rule(&rules->rule);
	return rules->rest;
}

fuspel* popn_fuspel(fuspel* rules, unsigned char n) {
	while (n > 0) {
		rules = pop_fuspel(rules);
		n--;
	}
	return rules;
}

void free_expression(expression* expr) {
	if (!expr)
		return;

	switch (expr->kind) {
		case EXPR_INT:
		case EXPR_NAME:
			my_free(expr->var1);
			break;
		case EXPR_CODE:
			my_free(expr->var2);
			break;
		case EXPR_LIST:
		case EXPR_TUPLE:
		case EXPR_APP:
			free_expression(expr->var1);
			free_expression(expr->var2);
			my_free(expr->var1);
			my_free(expr->var2);
			break;
	}

	expr->var1 = expr->var2 = NULL;
}

void free_arg_list(arg_list* list) {
	free_expression(&list->elem);
	if (list->rest)
		free_arg_list(list->rest);
	my_free(list->rest);
}

void free_rewrite_rule(rewrite_rule* rule) {
	my_free(rule->name);
	if (rule->args)
		free_arg_list(rule->args);
	my_free(rule->args);
	free_expression(&rule->rhs);
}

void free_fuspel(fuspel* rules) {
	free_rewrite_rule(&rules->rule);
	if (rules->rest)
		free_fuspel(rules->rest);
	my_free(rules->rest);
}
