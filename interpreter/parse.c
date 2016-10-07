#include "parse.h"

#include <string.h>

#include "code.h"
#include "mem.h"

extern struct fuspel *import(struct fuspel *already_parsed, char *name);

struct token_list *parse_name(char **name, struct token_list *list) {
	if (list->elem.kind != TOKEN_NAME)
		return NULL;

	*name = my_calloc(1, strlen(list->elem.var) + 1);

	strcpy(*name, list->elem.var);

	return list->rest;
}

struct token_list *parse_simple_expression(struct expression *expr, struct token_list *list) {
	struct expression *_expr;

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
				char *name;
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
					_expr = my_calloc(1, sizeof(struct expression));
					cpy_expression(_expr, expr);
					free_expression(expr);
					expr->kind = EXPR_TUPLE;
					expr->var1 = _expr;
					expr->var2 = my_calloc(1, sizeof(struct expression));
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

			expr->var1 = my_calloc(1, sizeof(struct expression));
			expr->var2 = my_calloc(1, sizeof(struct expression));

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

struct token_list *parse_arg_list(struct arg_list **args, struct token_list *list) {
	if (list->elem.kind == TOKEN_EQUALS)
		return list;

	*args = my_calloc(1, sizeof(struct arg_list));

	list = parse_simple_expression(&(*args)->elem, list);
	if (!list)
		return NULL;

	list = parse_arg_list(&(*args)->rest, list);

	return list;
}

struct token_list *parse_expression(struct expression*, struct token_list*);

struct token_list *parse_expression_no_app(struct expression *expr, struct token_list *list) {
	if (list->elem.kind == TOKEN_OPEN_P) {
		struct token_list *_list = parse_expression(expr, list->rest);
		if (_list) {
			if (_list->elem.kind == TOKEN_CLOSE_P) {
				return _list->rest;
			} else if (_list->elem.kind == TOKEN_COMMA) {
				struct expression *_expr = my_calloc(1, sizeof(struct expression));

				cpy_expression(_expr, expr);
				free_expression(expr);
				expr->kind = EXPR_TUPLE;
				expr->var1 = _expr;

				expr->var2 = my_calloc(1, sizeof(struct expression));

				_list = parse_expression(expr->var2, _list->rest);
				if (_list && _list->elem.kind == TOKEN_CLOSE_P) {
					return _list->rest;
				}
			}
			free_expression(expr);
		}
	} else if (list->elem.kind == TOKEN_OPEN_SQ) {
		struct token_list *_list;

		expr->kind = EXPR_LIST;

		if (list->rest->elem.kind == TOKEN_CLOSE_SQ) {
			return list->rest->rest;
		}

		expr->var1 = my_calloc(1, sizeof(struct expression));
		expr->var2 = my_calloc(1, sizeof(struct expression));

		_list = parse_expression(expr->var1, list->rest);
		if (!_list ||
				(_list->elem.kind != TOKEN_COLON &&
				 _list->elem.kind != TOKEN_COMMA &&
				 _list->elem.kind != TOKEN_CLOSE_SQ)) {
			free_expression(expr);
		} else if (_list->elem.kind == TOKEN_CLOSE_SQ) {
			((struct expression *) expr->var2)->kind = EXPR_LIST;
			((struct expression *) expr->var2)->var1 = NULL;
			((struct expression *) expr->var2)->var2 = NULL;
			return _list->rest;
		} else if (_list->elem.kind == TOKEN_COMMA) {
			struct expression *_expr = expr;
			while (_list && _list->elem.kind == TOKEN_COMMA) {
				_expr = _expr->var2;
				_expr->kind = EXPR_LIST;
				_expr->var1 = my_calloc(1, sizeof(struct expression));
				_expr->var2 = my_calloc(1, sizeof(struct expression));
				_list = parse_expression(_expr->var1, _list->rest);
			}
			if (!_list ||
					(_list->elem.kind != TOKEN_CLOSE_SQ &&
					 _list->elem.kind != TOKEN_COLON)) {
				free_expression(expr);
			} else if (_list->elem.kind == TOKEN_COLON) {
				_list = parse_expression(((struct expression*) expr->var2)->var2, _list->rest);
				if (!_list || _list->elem.kind != TOKEN_CLOSE_SQ) {
					free_expression(expr);
				} else {
					return _list->rest;
				}
			} else if (_list->elem.kind == TOKEN_CLOSE_SQ) {
				((struct expression*) _expr->var2)->kind = EXPR_LIST;
				((struct expression*) _expr->var2)->var1 = NULL;
				((struct expression*) _expr->var2)->var2 = NULL;
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

struct token_list *parse_expression(struct expression *expr, struct token_list *list) {
	list = parse_expression_no_app(expr, list);
	if (!list)
		return NULL;

	while (list->elem.kind != TOKEN_SEMICOLON &&
			list->elem.kind != TOKEN_CLOSE_P &&
			list->elem.kind != TOKEN_CLOSE_SQ &&
			list->elem.kind != TOKEN_COLON &&
			list->elem.kind != TOKEN_COMMA) {
	
		struct expression *_expr = my_calloc(1, sizeof(struct expression));

		cpy_expression(_expr, expr);
		free_expression(expr);
		expr->kind = EXPR_APP;
		expr->var1 = _expr;

		expr->var2 = my_calloc(1, sizeof(struct expression));

		list = parse_expression_no_app(expr->var2, list);
		if (!list) {
			free_expression(expr);
			return NULL;
		}
	}

	return list;
}

struct token_list *parse_rule(struct rewrite_rule *rule, struct token_list *list) {
	list = parse_name(&rule->name, list);
	if (!list)
		return NULL;

	list = parse_arg_list(&rule->args, list);
	if (!list) {
		my_free(rule->name);
		return NULL;
	}

	if (list->elem.kind != TOKEN_EQUALS) {
		my_free(rule->name);
		free_arg_list(rule->args);
		return NULL;
	}

	list = parse_expression(&rule->rhs, list->rest);

	return list;
}

struct fuspel *parse(struct token_list *list) {
	struct fuspel *rules = NULL;
	struct fuspel *return_rules;

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
		return_rules = rules = my_calloc(1, sizeof(struct fuspel));

		list = parse_rule(&rules->rule, list);
		if (!list)
			return NULL;
	}

	rules->rest = parse(list);

	return return_rules;
}
