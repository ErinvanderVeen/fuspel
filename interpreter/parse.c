#include "parse.h"

#include <string.h>

#include "code.h"
#include "log.h"
#include "mem.h"

extern fuspel* import(fuspel* already_parsed, char* name);

token_list* parse_name(char** name, token_list* list) {
	if (list->elem.kind != TOKEN_NAME)
		return NULL;

	*name = my_calloc(1, strlen(list->elem.var) + 1);

	strcpy(*name, list->elem.var);

	return list->rest;
}

token_list* parse_simple_expression(expression* expr, token_list* list) {
	expression* _expr;

	switch (list->elem.kind) {
		case TOKEN_INT:
			expr->kind = EXPR_INT;
			expr->var1 = my_calloc(1, sizeof(int));
			*((int*) expr->var1) = *((int*) list->elem.var);
			return list->rest;

		case TOKEN_NAME:
			expr->kind = EXPR_NAME;
			list = parse_name((char**) &expr->var1, list);
			return list;

		case TOKEN_CODE:
			if (list->rest && list->rest->elem.kind == TOKEN_NAME) {
				char* name;
				expr->kind = EXPR_CODE;
				list = parse_name(&name, list->rest);
				expr->var2 = my_calloc(1, sizeof(unsigned char));
				*((unsigned char*) expr->var2) = code_find(name, &expr->var1);
				my_free(name);
				return list;
			} else {
				return NULL;
			}

		case TOKEN_OPEN_P:
			list = parse_simple_expression(expr, list->rest);
			if (!list)
				return NULL;

			switch (list->elem.kind) {
				case TOKEN_CLOSE_P:
					return list->rest;
					break;
				case TOKEN_COMMA:
					_expr = my_calloc(1, sizeof(expression));
					cpy_expression(_expr, expr);
					free_expression(expr);
					expr->kind = EXPR_TUPLE;
					expr->var1 = _expr;
					expr->var2 = my_calloc(1, sizeof(expression));
					list = parse_simple_expression(expr->var2, list->rest);
					if (!list || list->elem.kind != TOKEN_CLOSE_P) {
						free_expression(_expr);
						my_free(_expr);
						return NULL;
					} else {
						return list->rest;
					}
					break;
				default:
					return NULL;
			}
			break;

		case TOKEN_OPEN_SQ:
			expr->kind = EXPR_LIST;
			list = list->rest;
			if (!list)
				return NULL;
			
			if (list->elem.kind == TOKEN_CLOSE_SQ)
				return list->rest;

			expr->var1 = my_calloc(1, sizeof(expression));
			expr->var2 = my_calloc(1, sizeof(expression));

			list = parse_simple_expression(expr->var1, list);

			if (!list || list->elem.kind != TOKEN_COLON) {
				free_expression(expr->var1);
				return NULL;
			}
			
			list = parse_simple_expression(expr->var2, list->rest);
				
			if (!list || list->elem.kind != TOKEN_CLOSE_SQ) {
				free_expression(expr->var1);
				my_free(expr->var1);
				free_expression(expr->var2);
				my_free(expr->var2);
				return NULL;
			}

			return list->rest;

			break;

		default:
			return NULL;
	}
}

token_list* parse_arg_list(arg_list** args, token_list* list) {
	if (list->elem.kind == TOKEN_EQUALS)
		return list;

	*args = my_calloc(1, sizeof(arg_list));

	list = parse_simple_expression(&(*args)->elem, list);
	if (!list)
		return NULL;

	list = parse_arg_list(&(*args)->rest, list);

	return list;
}

token_list* parse_expression(expression*, token_list*);

token_list* parse_expression_no_app(expression* expr, token_list* list) {
	if (list->elem.kind == TOKEN_OPEN_P) {
		token_list* _list = parse_expression(expr, list->rest);
		if (_list) {
			if (_list->elem.kind == TOKEN_CLOSE_P) {
				return _list->rest;
			} else if (_list->elem.kind == TOKEN_COMMA) {
				expression* _expr = my_calloc(1, sizeof(expression));

				cpy_expression(_expr, expr);
				free_expression(expr);
				expr->kind = EXPR_TUPLE;
				expr->var1 = _expr;

				expr->var2 = my_calloc(1, sizeof(expression));

				_list = parse_expression(expr->var2, _list->rest);
				if (_list && _list->elem.kind == TOKEN_CLOSE_P) {
					return _list->rest;
				}
			}
			free_expression(expr);
		}
	} else if (list->elem.kind == TOKEN_OPEN_SQ) {
		token_list* _list;

		expr->kind = EXPR_LIST;

		if (list->rest->elem.kind == TOKEN_CLOSE_SQ) {
			return list->rest->rest;
		}

		expr->var1 = my_calloc(1, sizeof(expression));
		expr->var2 = my_calloc(1, sizeof(expression));

		_list = parse_expression(expr->var1, list->rest);
		if (!_list ||
				(_list->elem.kind != TOKEN_COLON &&
				 _list->elem.kind != TOKEN_COMMA &&
				 _list->elem.kind != TOKEN_CLOSE_SQ)) {
			free_expression(expr);
		} else if (_list->elem.kind == TOKEN_CLOSE_SQ) {
			((expression *) expr->var2)->kind = EXPR_LIST;
			((expression *) expr->var2)->var1 = NULL;
			((expression *) expr->var2)->var2 = NULL;
			return _list->rest;
		} else if (_list->elem.kind == TOKEN_COMMA) {
			expression* _expr = expr;
			while (_list && _list->elem.kind == TOKEN_COMMA) {
				_expr = _expr->var2;
				_expr->kind = EXPR_LIST;
				_expr->var1 = my_calloc(1, sizeof(expression));
				_expr->var2 = my_calloc(1, sizeof(expression));
				_list = parse_expression(_expr->var1, _list->rest);
			}
			if (!_list ||
					(_list->elem.kind != TOKEN_CLOSE_SQ &&
					 _list->elem.kind != TOKEN_COLON)) {
				free_expression(expr);
			} else if (_list->elem.kind == TOKEN_COLON) {
				_list = parse_expression(((expression*) expr->var2)->var2, _list->rest);
				if (!_list || _list->elem.kind != TOKEN_CLOSE_SQ) {
					free_expression(expr);
				} else {
					return _list->rest;
				}
			} else if (_list->elem.kind == TOKEN_CLOSE_SQ) {
				((expression*) _expr->var2)->kind = EXPR_LIST;
				((expression*) _expr->var2)->var1 = NULL;
				((expression*) _expr->var2)->var2 = NULL;
				return _list->rest;
			}
		} else if (_list->elem.kind == TOKEN_COLON) {
			_list = parse_expression(expr->var2, _list->rest);
			if (!_list || _list->elem.kind != TOKEN_CLOSE_SQ) {
				free_expression(expr);
			} else {
				return _list->rest;
			}
		}
	}

	list = parse_simple_expression(expr, list);

	return list;
}

token_list* parse_expression(expression* expr, token_list* list) {
	list = parse_expression_no_app(expr, list);
	if (!list)
		return NULL;

	while (list->elem.kind != TOKEN_SEMICOLON &&
			list->elem.kind != TOKEN_CLOSE_P &&
			list->elem.kind != TOKEN_CLOSE_SQ &&
			list->elem.kind != TOKEN_COLON &&
			list->elem.kind != TOKEN_COMMA) {
	
		expression* _expr = my_calloc(1, sizeof(expression));

		cpy_expression(_expr, expr);
		free_expression(expr);
		expr->kind = EXPR_APP;
		expr->var1 = _expr;

		expr->var2 = my_calloc(1, sizeof(expression));

		list = parse_expression_no_app(expr->var2, list);
		if (!list) {
			free_expression(expr);
			return NULL;
		}
	}

	return list;
}

token_list* parse_rule(rewrite_rule* rule, token_list* list) {
	list = parse_name(&rule->name, list);
	if (!list)
		return NULL;

	list = parse_arg_list(&rule->args, list);
	if (!list) {
		log_debug("parse_rule: error in arg_list");
		my_free(rule->name);
		return NULL;
	}

	if (list->elem.kind != TOKEN_EQUALS) {
		log_debug("parse_rule: no =");
		my_free(rule->name);
		free_arg_list(rule->args);
		return NULL;
	}

	list = parse_expression(&rule->rhs, list->rest);

	return list;
}

fuspel* parse(token_list* list) {
	fuspel* rules = NULL;
	fuspel* return_rules;

	while (list && list->elem.kind == TOKEN_SEMICOLON)
		list = list->rest;

	if (!list)
		return NULL;

	if (list->elem.kind == TOKEN_IMPORT) {
		list = list->rest;
		if (!list || list->elem.kind != TOKEN_NAME)
			return NULL;
		rules = import(rules, list->elem.var);
		if (!rules)
			return NULL;

		list = list->rest->rest;

		return_rules = rules;
		while (rules->rest) rules = rules->rest;
	} else {
		return_rules = rules = my_calloc(1, sizeof(fuspel));

		list = parse_rule(&rules->rule, list);
		if (!list)
			return NULL;
	}

	rules->rest = parse(list);

	return return_rules;
}
