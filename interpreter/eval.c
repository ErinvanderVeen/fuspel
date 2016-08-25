#include "eval.h"

#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "print.h"

void free_rules_until(fuspel* new, fuspel* old) {
	while (new != old) {
		fuspel* _new = new->rest;
		free_rewrite_rule(&new->rule);
		my_free(new);
		new = _new;
	}
}

typedef struct replacements {
	char* what;
	expression* with;
	struct replacements* rest;
} replacements;

void replace(char* name, expression* new, expression* expr) {
	if (!expr)
		return;

	switch (expr->kind) {
		case EXPR_NAME:
			if (!strcmp(expr->var1, name)) {
				cpy_expression(expr, new);
				break;
			}
		case EXPR_INT:
			break;
		case EXPR_TUPLE:
		case EXPR_LIST:
		case EXPR_APP:
			replace(name, new, expr->var1);
			replace(name, new, expr->var2);
			break;
	}
}

void replace_all(replacements* repls, expression* expr) {
	while (repls) {
		replace(repls->what, repls->with, expr);
		repls = repls->rest;
	}
}

replacements* push_replacement(char* what, expression* with, replacements* rest) {
	replacements* new = my_calloc(1, sizeof(replacements));
	new->what = what;
	new->with = my_calloc(1, sizeof(expression));
	cpy_expression(new->with, with);
	new->rest = rest;
	return new;
}

void free_replacements(replacements* repls) {
	if (repls) {
		free_replacements(repls->rest);
		repls->rest = NULL;
		free_expression(repls->with);
		my_free(repls->with);
		my_free(repls);
	}
}

unsigned match_expr(fuspel* rules, expression* to_match, expression* expr,
		replacements** repls) {
	unsigned matches;

	if (to_match->kind != EXPR_NAME) {
		expr = eval_rnf(rules, expr);
		if (!expr)
			return 0;
	}

	switch (to_match->kind) {
		case EXPR_NAME:
			*repls = push_replacement(to_match->var1, expr, *repls);
			free_expression(expr);
			my_free(expr);
			return 1;
		case EXPR_INT:
			matches = eq_expression(to_match, expr);
			free_expression(expr);
			my_free(expr);
			return matches;
		case EXPR_LIST:
			if (!to_match->var1) {
				matches = eq_expression(to_match, expr);
				free_expression(expr);
				my_free(expr);
				return matches;
			}
		case EXPR_TUPLE:
			if (to_match->kind != expr->kind) {
				free_expression(expr);
				my_free(expr);
				return 0;
			}
			matches =
				match_expr(rules, to_match->var1, expr->var1, repls) &&
				match_expr(rules, to_match->var2, expr->var2, repls);
			free_expression(expr);
			my_free(expr);
			return matches;
		default:
			free_expression(expr);
			my_free(expr);
			return 0;
	}
}

unsigned match_rule(fuspel* rules, rewrite_rule* rule, expression* expr,
		replacements** repls) {
	expression** expr_args;
	unsigned char i;

	switch (expr->kind) {
		case EXPR_NAME:
			return (!strcmp(expr->var1, rule->name) &&
				empty_args_list(rule->args)) ? 1 : 0;
		case EXPR_APP:
			expr_args = flatten_app_args(expr);
			i = 0;
			if (!strcmp(expr_args[0]->var1, rule->name)) {
				expression* _expr = expr_args[++i];
				arg_list* args = rule->args;
				fuspel* _rules = rules;
				while (!empty_args_list(args)) {
					if (!match_expr(_rules, &args->elem, _expr, repls)) {
						free_rules_until(_rules, rules);
						my_free(expr_args);
						return 0;
					}

					args = args->rest;
					_expr = expr_args[++i];

					if (!empty_args_list(args) && !_expr) {
						free_rules_until(_rules, rules);
						my_free(expr_args);
						return 0;
					}
				}
				my_free(expr_args);
				return 1;
			}
			my_free(expr_args);
		default:
			return 0;
	}
}

expression* eval_rnf(fuspel* rules, expression* expr) {
	expression* result = my_calloc(1, sizeof(expression));

	fuspel* _rules = rules;

	replacements** repls = my_calloc(1, sizeof(replacements*));

	switch (expr->kind) {
		case EXPR_INT:
		case EXPR_TUPLE:
		case EXPR_LIST:
			cpy_expression(result, expr);
			break;

		case EXPR_NAME:
		case EXPR_APP:
			while (_rules) {
				if (match_rule(rules, &_rules->rule, expr, repls)) {
					expression* old_result = result;
					cpy_expression(result, &_rules->rule.rhs);
					replace_all(*repls, result);
					free_replacements(*repls);
					my_free(repls);
					result = eval_rnf(rules, old_result);
					free_expression(old_result);
					my_free(old_result);
					return result;
				}
				free_replacements(*repls);
				my_free(repls);
				repls = my_calloc(1, sizeof(replacements*));
				_rules = _rules->rest;
			}
			cpy_expression(result, expr);
			break;
	}

	free_replacements(*repls);
	my_free(repls);

	return result;
}

expression* eval(fuspel* rules, expression* expr) {
	expression *e1, *e2;
	fuspel* _rules = rules;
	expression* result = my_calloc(1, sizeof(expression));
	replacements** repls = my_calloc(1, sizeof(replacements*));

	printf("Evaluating: ");
	print_expression(expr);
	printf("\n");

	switch (expr->kind) {
		case EXPR_INT:
			cpy_expression(result, expr);
			break;

		case EXPR_NAME:
		case EXPR_APP:
			while (_rules) {
				if (match_rule(rules, &_rules->rule, expr, repls)) {
					expression* old_result = result;
					cpy_expression(result, &_rules->rule.rhs);
					replace_all(*repls, result);
					result = eval(rules, old_result);
					free_expression(old_result);
					my_free(old_result);
					free_replacements(*repls);
					my_free(repls);
					return result;
				}
				free_replacements(*repls);
				my_free(repls);
				repls = my_calloc(1, sizeof(replacements*));
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

	free_replacements(*repls);
	my_free(repls);

	return result;
}
