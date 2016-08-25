#include "syntax.h"

#include <stdlib.h>
#include <string.h>

void free_token(token* tk) {
	if (tk->var) {
		free(tk->var);
	}
}

void free_token_list(token_list* list) {
	if (list->rest) {
		free_token_list(list->rest);
	}
	free_token(&list->elem);
}

unsigned empty_args_list(arg_list* list) {
	return !list;
}

void cpy_expression(expression* dst, expression* src) {
	dst->kind = src->kind;
	switch (dst->kind) {
		case EXPR_INT:
			dst->var1 = malloc(sizeof(int));
			if (!dst->var1)
				error_no_mem();
			*((int*) dst->var1) = *((int*) src->var1);
			break;
		case EXPR_NAME:
			dst->var1 = malloc(strlen((char*) src->var1) + 1);
			if (!dst->var1)
				error_no_mem();
			strcpy(dst->var1, src->var1);
			break;
		case EXPR_LIST:
			if (!src->var1)
				break;
		case EXPR_TUPLE:
		case EXPR_APP:
			dst->var1 = malloc(sizeof(expression));
			dst->var2 = malloc(sizeof(expression));
			if (!dst->var1 || !dst->var2)
				error_no_mem();
			cpy_expression(dst->var1, src->var1);
			cpy_expression(dst->var2, src->var2);
			break;
	}
}

unsigned eq_expression(expression* a, expression* b) {
	if (a->kind != b->kind)
		return 0;

	switch (a->kind) {
		case EXPR_INT:  return *((int*) a->var1) == *((int*) b->var1);
		case EXPR_NAME: return !strcmp(a->var1, b->var1);
		case EXPR_TUPLE:
		case EXPR_LIST:
		case EXPR_APP:
			if (!a->var1 && b->var1 || a->var1 && !b->var1 ||
					!a->var2 && b->var2 || a->var2 && b->var2)
				return 0;
			if (a->var1 && !eq_expression(a->var1, b->var1))
				return 0;
			if (a->var2 && !eq_expression(a->var2, b->var2))
				return 0;
			return 1;
	}
}

expression** flatten_app_args(expression* from) {
	unsigned char len = 0;
	expression* _from = from;
	while (_from->kind == EXPR_APP) {
		len++;
		_from = _from->var1;
	}
	len++;

	expression** result = calloc(1, sizeof(expression*) * (len + 1));
	unsigned int i = 1;
	while (from->kind == EXPR_APP) {
		result[len - i] = from->var2;
		from = from->var1;
		i++;
	}
	result[0] = from;
	result[len] = NULL;
	return result;
}

void concat_fuspel(fuspel* start, fuspel* end) {
	do {
		if (!start->rest) {
			start->rest = end;
			return;
		}
		start = start->rest;
	} while (start);
}

fuspel* push_fuspel(fuspel* rules) {
	fuspel* new_rules = calloc(1, sizeof(fuspel));
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
			free(expr->var1);
			break;
		case EXPR_LIST:
		case EXPR_TUPLE:
		case EXPR_APP:
			free_expression(expr->var1);
			free_expression(expr->var2);
			break;
	}
}

void free_arg_list(arg_list* list) {
	if (list->rest)
		free_arg_list(list->rest);
	free_expression(&list->elem);
}

void free_rewrite_rule(rewrite_rule* rule) {
	free(rule->name);
	if (rule->args)
		free_arg_list(rule->args);
	free_expression(&rule->rhs);
}
