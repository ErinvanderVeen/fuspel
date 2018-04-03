#include "parse.h"

#include <stdbool.h>
#include <string.h>

#include "code.h"
#include "mem.h"

extern struct fuspel *import(struct fuspel *already_parsed, char *name);

struct parsing_list {
	struct token_list *tokens;
	unsigned int i;
};

bool parse_name(char **name, struct parsing_list *list) {
	if (list->tokens->elems[list->i].kind != TOKEN_NAME)
		return false; // TODO error handling

	*name = my_calloc(1, strlen(list->tokens->elems[list->i].var) + 1);

	strcpy(*name, list->tokens->elems[list->i].var);

	list->i++;

	return true;
}

bool parse_simple_expression(struct expression *expr, struct parsing_list *list) {
	struct expression *_expr;

	switch (list->tokens->elems[list->i].kind) {
		case TOKEN_INT:
			expr->kind = EXPR_INT;
			expr->var1 = list->tokens->elems[list->i].var;
			list->i++;
			return true;

		case TOKEN_NAME:
			expr->kind = EXPR_NAME;
			parse_name((char**) &expr->var1, list);
			return true;

		case TOKEN_CODE:
			if (list->i < list->tokens->index &&
					list->tokens->elems[list->i+1].kind == TOKEN_NAME) {
				char *name;
				expr->kind = EXPR_CODE;
				list->i++;
				parse_name(&name, list);
				expr->var2 = my_calloc(1, sizeof(unsigned char));
				*((unsigned char*) expr->var2) = code_find(name, &expr->var1);
				my_free(name);
				return true;
			} else {
				return false; // TODO error handling
			}

		case TOKEN_OPEN_P:
			list->i++;
			if (!parse_simple_expression(expr, list)) {
				list->i--;
				return false;
			}

			switch (list->tokens->elems[list->i].kind) {
				case TOKEN_CLOSE_P:
					list->i++;
					return true;
				case TOKEN_COMMA:
					_expr = my_calloc(1, sizeof(struct expression));
					cpy_expression(_expr, expr);
					free_expression(expr);
					expr->kind = EXPR_TUPLE;
					expr->var1 = _expr;
					expr->var2 = my_calloc(1, sizeof(struct expression));
					list->i++;
					if (!parse_simple_expression(expr->var2, list) ||
							list->tokens->elems[list->i].kind != TOKEN_CLOSE_P) {
						list->i--;
						free_expression(_expr);
						my_free(_expr);
						return false;
					} else {
						list->i++;
						return true;
					}
				default:
					return false;
			}
			break;

		case TOKEN_OPEN_SQ:
			expr->kind = EXPR_LIST;
			list->i++;
			
			if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_SQ) {
				list->i++;
				return true;
			}

			expr->var1 = my_calloc(1, sizeof(struct expression));
			expr->var2 = my_calloc(1, sizeof(struct expression));

			if (!parse_simple_expression(expr->var1, list) ||
					(list->tokens->elems[list->i].kind != TOKEN_COLON &&
					 list->tokens->elems[list->i].kind != TOKEN_COMMA &&
					 list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ)) {
				free_expression(expr->var1);
				return false;
			} else if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_SQ) {
				((struct expression *) expr->var2)->kind = EXPR_LIST;
				((struct expression *) expr->var2)->var1 = NULL;
				((struct expression *) expr->var2)->var2 = NULL;
				list->i++;
				return true;
			} else if (list->tokens->elems[list->i].kind == TOKEN_COMMA) {
				struct expression *_expr = expr;
				bool loop = true;
				while (loop && list->tokens->elems[list->i].kind == TOKEN_COMMA) {
					_expr = _expr->var2;
					_expr->kind = EXPR_LIST;
					_expr->var1 = my_calloc(1, sizeof(struct expression));
					_expr->var2 = my_calloc(1, sizeof(struct expression));
					list->i++;
					loop = parse_simple_expression(_expr->var1, list);
				}
				if (list->i >= list->tokens->length ||
						(list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ &&
						 list->tokens->elems[list->i].kind != TOKEN_COLON)) {
					free_expression(expr);
					return false;
				} else if (list->tokens->elems[list->i].kind == TOKEN_COLON) {
					list->i++;
					if (!parse_simple_expression(((struct expression*) expr->var2)->var2, list) ||
							list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ) {
						free_expression(expr);
						return false;
					} else {
						list->i++;
						return true;
					}
				} else if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_SQ) {
					((struct expression*) _expr->var2)->kind = EXPR_LIST;
					((struct expression*) _expr->var2)->var1 = NULL;
					((struct expression*) _expr->var2)->var2 = NULL;
					list->i++;
					return true;
				}
			} else if (list->tokens->elems[list->i].kind == TOKEN_COLON) {
				list->i++;
				if (!parse_simple_expression(expr->var2, list) ||
						list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ) {
					free_expression(expr);
					return false;
				} else {
					list->i++;
					return true;
				}
			}

		default:
			return false;
	}
}

bool parse_arg_list(struct arg_list **args, struct parsing_list *list) {
	if (list->tokens->elems[list->i].kind == TOKEN_EQUALS)
		return true;

	*args = my_calloc(1, sizeof(struct arg_list));

	if (!parse_simple_expression(&(*args)->elem, list))
		return false;

	return parse_arg_list(&(*args)->rest, list);
}

bool parse_expression(struct expression*, struct parsing_list*);

bool parse_expression_no_app(struct expression *expr, struct parsing_list *list) {
	if (list->tokens->elems[list->i].kind == TOKEN_OPEN_P) {
		list->i++;
		if (parse_expression(expr, list)) {
			if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_P) {
				list->i++;
				return true;
			} else if (list->tokens->elems[list->i].kind == TOKEN_COMMA) {
				struct expression *_expr = my_calloc(1, sizeof(struct expression));

				cpy_expression(_expr, expr);
				free_expression(expr);
				expr->kind = EXPR_TUPLE;
				expr->var1 = _expr;

				expr->var2 = my_calloc(1, sizeof(struct expression));

				list->i++;
				if (parse_expression(expr->var2, list) &&
						list->tokens->elems[list->i].kind == TOKEN_CLOSE_P) {
					list->i++;
					return true;
				}
			}
			free_expression(expr);
		}
	} else if (list->tokens->elems[list->i].kind == TOKEN_OPEN_SQ) {
		expr->kind = EXPR_LIST;

		if (list->tokens->elems[list->i+1].kind == TOKEN_CLOSE_SQ) {
			list->i += 2;
			return true;
		}

		expr->var1 = my_calloc(1, sizeof(struct expression));
		expr->var2 = my_calloc(1, sizeof(struct expression));

		list->i++;
		if (!parse_expression(expr->var1, list) ||
				(list->tokens->elems[list->i].kind != TOKEN_COLON &&
				 list->tokens->elems[list->i].kind != TOKEN_COMMA &&
				 list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ)) {
			free_expression(expr);
		} else if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_SQ) {
			((struct expression *) expr->var2)->kind = EXPR_LIST;
			((struct expression *) expr->var2)->var1 = NULL;
			((struct expression *) expr->var2)->var2 = NULL;
			list->i++;
			return true;
		} else if (list->tokens->elems[list->i].kind == TOKEN_COMMA) {
			struct expression *_expr = expr;
			bool loop = true;
			while (loop && list->tokens->elems[list->i].kind == TOKEN_COMMA) {
				_expr = _expr->var2;
				_expr->kind = EXPR_LIST;
				_expr->var1 = my_calloc(1, sizeof(struct expression));
				_expr->var2 = my_calloc(1, sizeof(struct expression));
				list->i++;
				loop = parse_expression(_expr->var1, list);
			}
			if (list->i >= list->tokens->length ||
					(list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ &&
					 list->tokens->elems[list->i].kind != TOKEN_COLON)) {
				free_expression(expr);
			} else if (list->tokens->elems[list->i].kind == TOKEN_COLON) {
				list->i++;
				if (!parse_expression(((struct expression*) expr->var2)->var2, list) ||
						list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ)
					free_expression(expr);
				else
					return list;
			} else if (list->tokens->elems[list->i].kind == TOKEN_CLOSE_SQ) {
				((struct expression*) _expr->var2)->kind = EXPR_LIST;
				((struct expression*) _expr->var2)->var1 = NULL;
				((struct expression*) _expr->var2)->var2 = NULL;
				list->i++;
				return list;
			}
		} else if (list->tokens->elems[list->i].kind == TOKEN_COLON) {
			list->i++;
			if (!parse_expression(expr->var2, list) ||
					list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ) {
				free_expression(expr);
			} else {
				list->i++;
				return list;
			}
		}
	}

	return parse_simple_expression(expr, list);
}

bool parse_expression(struct expression *expr, struct parsing_list *list) {
	if (!parse_expression_no_app(expr, list))
		return false;

	while (list->tokens->elems[list->i].kind != TOKEN_SEMICOLON &&
			list->tokens->elems[list->i].kind != TOKEN_CLOSE_P &&
			list->tokens->elems[list->i].kind != TOKEN_CLOSE_SQ &&
			list->tokens->elems[list->i].kind != TOKEN_COLON &&
			list->tokens->elems[list->i].kind != TOKEN_COMMA) {
		struct expression *_expr = my_calloc(1, sizeof(struct expression));

		cpy_expression(_expr, expr);
		free_expression(expr);
		expr->kind = EXPR_APP;
		expr->var1 = _expr;

		expr->var2 = my_calloc(1, sizeof(struct expression));

		if (!parse_expression_no_app(expr->var2, list)) {
			free_expression(expr);
			return false;
		}
	}

	return true;
}

bool parse_rule(struct rewrite_rule *rule, struct parsing_list *list) {
	if (!parse_name(&rule->name, list))
		return false;

	if (!parse_arg_list(&rule->args, list)) {
		my_free(rule->name);
		return false;
	}

	if (list->tokens->elems[list->i].kind != TOKEN_EQUALS) {
		my_free(rule->name);
		free_arg_list(rule->args);
		return false;
	}

	list->i++;
	return parse_expression(&rule->rhs, list);
}

struct fuspel *parse_intern(struct parsing_list *list) {
	struct fuspel *rules = NULL;
	struct fuspel *return_rules;

	while (list->i < list->tokens->index &&
			list->tokens->elems[list->i].kind == TOKEN_SEMICOLON)
		list->i++;

	if (list->i >= list->tokens->index)
		return NULL;

	if (list->tokens->elems[list->i].kind == TOKEN_IMPORT) {
		list->i++;
		if (!list || list->tokens->elems[list->i].kind != TOKEN_NAME)
			return NULL;
		rules = import(rules, list->tokens->elems[list->i].var);
		if (!rules)
			return NULL;

		list->i += 2;

		return_rules = rules;
		while (rules->rest) rules = rules->rest;
	} else {
		return_rules = rules = my_calloc(1, sizeof(struct fuspel));

		if (!parse_rule(&rules->rule, list)) {
			my_free(rules);
			return NULL;
		}
	}

	rules->rest = parse_intern(list);

	return return_rules;
}

struct fuspel *parse(struct token_list *tk_list) {
	struct fuspel *pgm;
	struct parsing_list *list = my_calloc(1, sizeof(struct parsing_list));
	list->i = 0;
	list->tokens = tk_list;
	pgm = parse_intern(list);
	my_free(list);
	return pgm;
}
