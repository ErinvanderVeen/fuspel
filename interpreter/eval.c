#include "eval.h"

#include <string.h>

#include "code.h"
#include "mem.h"

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
	unsigned is_strict;
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
		case EXPR_CODE:
			break;
		case EXPR_TUPLE:
		case EXPR_LIST:
		case EXPR_APP:
			replace(name, new, expr->var1);
			replace(name, new, expr->var2);
			break;
	}
}

void replace_all(fuspel* rules, replacements* repls, expression* expr) {
	while (repls) {
		if (repls->is_strict) {
			expression* temp = eval(rules, repls->with);
			free_expression(repls->with);
			my_free(repls->with);
			repls->with = temp;
		}
		replace(repls->what, repls->with, expr);
		repls = repls->rest;
	}
}

replacements* push_replacement(char* what, expression* with,
		unsigned is_strict, replacements* rest) {
	replacements* new = my_calloc(1, sizeof(replacements));
	new->what = what;
	new->with = my_calloc(1, sizeof(expression));
	cpy_expression(new->with, with);
	new->is_strict = is_strict;
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

unsigned match_expr(fuspel* rules, expression* arg, expression* expr,
		replacements** repls, unsigned is_strict) {
	unsigned matches;

	if (arg->kind != EXPR_NAME) {
		expr = eval_rnf(rules, expr);
		if (!expr)
			return 0;
	}

	is_strict |= arg->is_strict;

	switch (arg->kind) {
		case EXPR_NAME:
			*repls = push_replacement(
					arg->var1, expr, is_strict, *repls);
			return 1;
		case EXPR_INT:
			matches = eq_expression(arg, expr);
			free_expression(expr);
			my_free(expr);
			return matches;
		case EXPR_LIST:
			if (!arg->var1) {
				matches = eq_expression(arg, expr);
				free_expression(expr);
				my_free(expr);
				return matches;
			}
		case EXPR_TUPLE:
			if (arg->kind != expr->kind) {
				free_expression(expr);
				my_free(expr);
				return 0;
			}
			matches =
				match_expr(rules, arg->var1, expr->var1, repls, is_strict) &&
				match_expr(rules, arg->var2, expr->var2, repls, is_strict);
			free_expression(expr);
			my_free(expr);
			return matches;
		default:
			free_expression(expr);
			my_free(expr);
			return 0;
	}
}

/**
 * Return value:
 *   < 0: rule does not match expr
 *     n: rule matches expr; n arguments at the end cannot be touched
 *
 * E.g., a rule
 *   f x y = (x,y)
 * can be applied to
 *   f 1 2 3 4
 * but 2 arguments cannot be touched (3 and 4).
 */
char match_rule(fuspel* rules, rewrite_rule* rule, expression* expr,
		replacements** repls) {
	expression** expr_args;
	unsigned char i;

	switch (expr->kind) {
		case EXPR_NAME:
			return (!strcmp(expr->var1, rule->name) &&
				empty_args_list(rule->args)) ? 0 : -1;
		case EXPR_APP:
			expr_args = flatten_app_args(expr);
			i = 0;
			if (!strcmp(expr_args[0]->var1, rule->name)) {
				expression* _expr = expr_args[++i];
				arg_list* args = rule->args;
				unsigned char args_len = len_arg_list(args);

				while (!empty_args_list(args)) {
					if (!match_expr(rules, &args->elem, _expr, repls, 0)) {
						my_free(expr_args);
						return -1;
					}

					args = args->rest;
					_expr = expr_args[++i];

					if (!empty_args_list(args) && !_expr) {
						my_free(expr_args);
						return -1;
					}
				}
				while (_expr) _expr = expr_args[++i];
				my_free(expr_args);
				return i - args_len - 1;
			}
			my_free(expr_args);
		default:
			return -1;
	}
}

/**
 * If
 *   app:  f x y
 *   from: g 1 2 3 4
 *   n:    3
 *
 * Then the result will be:
 *         f x y 2 3 4
 */
expression* append_to_app(expression* app, expression* from, unsigned char n) {
	expression *_from, *_app;
	unsigned char i;
	for (; n > 0; n--) {
		_app = my_calloc(1, sizeof(expression));
		_app->kind = EXPR_APP;
		_app->var1 = app;
		_app->var2 = my_calloc(1, sizeof(expression));

		_from = from;
		for (i = 1; i < n; i++) _from = _from->var1;
		cpy_expression(_app->var2, _from->var2);

		app = _app;
	}
	return app;
}

unsigned is_code_app(expression* expr) {
	for (; expr->kind == EXPR_APP; expr = expr->var1);
	return expr->kind == EXPR_CODE;
}

expression* eval_code(fuspel* rules, expression* expr) {
	expression *root, *result, **args;
	Code_1* f1; Code_2* f2;
	expression *a1, *a2;
	unsigned char args_len;

	a1 = a2 = NULL;

	for (root = expr; root->kind == EXPR_APP; root = root->var1);
	if (root->kind != EXPR_CODE || !root->var1) {
		return NULL;
	}

	args = flatten_app_args(expr);

	switch (*((unsigned char*) root->var2)) {
		case 1:
			f1 = (Code_1*) root->var1;
			a1 = eval(rules, args[1]);
			if (!a1) {
				my_free(a1);
				my_free(args);
				return NULL;
			}
			result = f1(a1);
			break;
		case 2:
			f2 = (Code_2*) root->var1;
			a1 = eval(rules, args[1]);
			a2 = eval(rules, args[2]);
			if (!a1 || !a2) {
				free_expression(a1);
				free_expression(a2);
				my_free(a1);
				my_free(a2);
				my_free(args);
				return NULL;
			}
			result = f2(a1, a2);
			break;
	}

	for (args_len = 0; args[args_len]; args_len++);
	result = append_to_app(result, expr, args_len - *((unsigned char*) root->var2) - 1);

	my_free(args);
	if (a1) free_expression(a1);
	if (a2) free_expression(a2);
	my_free(a1);
	my_free(a2);

	return result;
}

expression* eval_rnf(fuspel* rules, expression* expr) {
	expression* result = my_calloc(1, sizeof(expression));

	fuspel* _rules = rules;

	replacements** repls = my_calloc(1, sizeof(replacements*));

	switch (expr->kind) {
		case EXPR_INT:
		case EXPR_TUPLE:
		case EXPR_LIST:
		case EXPR_CODE:
			cpy_expression(result, expr);
			break;

		case EXPR_NAME:
		case EXPR_APP:
			if (is_code_app(expr)) {
				expression *_result;
				my_free(result);
				_result = eval_code(rules, expr);
				result = eval_rnf(rules, _result);
				free_expression(_result);
				my_free(_result);
				break;
			}

			while (_rules) {
				char skip_args;
				if ((skip_args = match_rule(
								rules, &_rules->rule, expr, repls)) >= 0) {
					expression *old_result;
					result = append_to_app(result, expr, skip_args);
					old_result = result;
					cpy_expression(result, &_rules->rule.rhs);
					replace_all(rules, *repls, result);
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

	switch (expr->kind) {
		case EXPR_INT:
			cpy_expression(result, expr);
			break;

		case EXPR_CODE:
			if (*((unsigned char*) expr->var2) == 0) {
				Code_0* code_fun = (Code_0*) expr->var1;
				my_free(result);
				result = code_fun();
			} else {
				cpy_expression(result, expr);
			}
			break;

		case EXPR_NAME:
		case EXPR_APP:
			if (is_code_app(expr)) {
				expression* _result;
				my_free(result);
				_result = eval_code(rules, expr);
				result = eval(rules, _result);
				free_expression(_result);
				my_free(_result);
				break;
			}

			while (_rules) {
				char skip_args;
				if ((skip_args = match_rule(
								rules, &_rules->rule, expr, repls)) >= 0) {
					expression *old_result;
					cpy_expression(result, &_rules->rule.rhs);
					replace_all(rules, *repls, result);
					result = append_to_app(result, expr, skip_args);
					old_result = result;
					result = eval(rules, result);
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

expression* eval_main(fuspel* rules) {
	expression to_eval, *evaled;

	to_eval.kind = EXPR_NAME;
	to_eval.var1 = my_calloc(1, 5);
	strcpy(to_eval.var1, "main");

	evaled = eval(rules, &to_eval);

	free_expression(&to_eval);

	return evaled;
}
