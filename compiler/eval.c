#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

void free_rules_until(fuspel* new, fuspel* old) {
	while (new != old) {
		free_rewrite_rule(&new->rule);
		new = new->rest;
	}
}

fuspel* match_expr(fuspel* rules, expression* to_match, expression* expr) {
	if (to_match->kind != EXPR_NAME) {
		expr = eval_rnf(rules, expr);
		if (!expr)
			return NULL;
	}

	switch (to_match->kind) {
		case EXPR_NAME:
			rules = push_fuspel(rules);
			rules->rule.name = malloc(strlen(to_match->var1) + 1);
			if (!rules->rule.name)
				error_no_mem();
			strcpy(rules->rule.name, to_match->var1);
			rules->rule.args = NULL;
			cpy_expression(&rules->rule.rhs, expr);
			return rules;
		case EXPR_INT:
			;unsigned matches = eq_expression(to_match, expr);
			free_expression(expr);
			free(expr);
			return matches ? rules : NULL;
		case EXPR_LIST:
			if (!to_match->var1) { // empty list
				unsigned matches = eq_expression(to_match, expr);
				free_expression(expr);
				free(expr);
				return matches ? rules : NULL;
			}
		case EXPR_TUPLE:
			;fuspel* _rules = match_expr(rules, to_match->var1, expr->var1);
			if (!_rules) {
				free_expression(expr);
				free(expr);
				return NULL;
			}
			fuspel* __rules = match_expr(_rules, to_match->var2, expr->var2);
			if (!__rules)
				free_rules_until(_rules, rules);
			free_expression(expr);
			free(expr);
			return __rules;
		default:
			free_expression(expr);
			free(expr);
			return NULL;
	}
}

fuspel* match_rule(fuspel* rules, rewrite_rule* rule, expression* expr) {
	switch (expr->kind) {
		case EXPR_NAME:
			return (!strcmp(expr->var1, rule->name) &&
				empty_args_list(rule->args)) ? rules : NULL;
		case EXPR_APP:
			;expression** expr_args = flatten_app_args(expr);
			unsigned char i = 0;
			if (!strcmp(expr_args[0]->var1, rule->name)) {
				expression* _expr = expr_args[++i];
				arg_list* args = rule->args;
				fuspel* _rules = rules;
				while (!empty_args_list(args)) {
					fuspel* __rules = match_expr(_rules, &args->elem, _expr);
					if (!__rules) {
						free_rules_until(_rules, rules);
						free(expr_args);
						return NULL;
					}
					_rules = __rules;

					args = args->rest;
					_expr = expr_args[++i];

					if (!empty_args_list(args) && !_expr) {
						free_rules_until(_rules, rules);
						return NULL;
					}
				}
				free(expr_args);
				return _rules;
			}
		default:
			return NULL;
	}
}

unsigned apply(expression* result, rewrite_rule* rule, expression* expr) {
	switch (expr->kind) {
		case EXPR_NAME:
			cpy_expression(result, &rule->rhs);
			return 1;
			break;
		case EXPR_APP:
			// TODO
		default:
			return 0;
	}
}

expression* eval_rnf(fuspel* rules, expression* expr) {
	expression* result = calloc(1, sizeof(expression));
	if (!result)
		error_no_mem();

	expression *e1, *e2;
	fuspel* _rules = rules;
	fuspel* new_rules;

	switch (expr->kind) {
		case EXPR_INT:
		case EXPR_TUPLE:
		case EXPR_LIST:
			cpy_expression(result, expr);
			break;

		case EXPR_NAME:
		case EXPR_APP:
			while (_rules) {
				new_rules = match_rule(rules, &_rules->rule, expr);
				if (new_rules) {
					rules = new_rules;
					result = eval_rnf(rules, &_rules->rule.rhs);
					free_rules_until(new_rules, _rules);
					return result;
				}
				_rules = _rules->rest;
			}
			cpy_expression(result, expr);
			break;
	}

	return result;
}

expression* eval(fuspel* rules, expression* expr) {
	expression* result = calloc(1, sizeof(expression));
	if (!result)
		error_no_mem();

	expression *e1, *e2;
	fuspel* _rules = rules;
	fuspel* new_rules;

	switch (expr->kind) {
		case EXPR_INT:
			cpy_expression(result, expr);
			break;

		case EXPR_NAME:
		case EXPR_APP:
			while (_rules) {
				new_rules = match_rule(rules, &_rules->rule, expr);
				if (new_rules) {
					rules = new_rules;
					result = eval(new_rules, &_rules->rule.rhs);
					free_rules_until(new_rules, rules);
					return result;
				}
				_rules = _rules->rest;
			}
			cpy_expression(result, expr);
			break;

		case EXPR_LIST:
			if (!expr->var1) {
				cpy_expression(result, expr);
				break;
			}
		case EXPR_TUPLE:
			e1 = eval(rules, expr->var1);
			e2 = eval(rules, expr->var2);

			result->kind = expr->kind;
			result->var1 = e1;
			result->var2 = e2;
			break;
	}

	return result;
}
