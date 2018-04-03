#ifndef _H_FUSPEL_SYNTAX
#define _H_FUSPEL_SYNTAX

#include <stdbool.h>

typedef long int INT;

/* TOKENS */

enum token_kind {
	TOKEN_SEMICOLON, /* ; */
	TOKEN_EQUALS,    /* = */
	TOKEN_OPEN_SQ,   /* [ */
	TOKEN_CLOSE_SQ,  /* ] */
	TOKEN_OPEN_P,    /* ( */
	TOKEN_CLOSE_P,   /* ) */
	TOKEN_COMMA,     /* , */
	TOKEN_COLON,     /* : */
	TOKEN_CODE,      /* code */
	TOKEN_IMPORT,    /* import */
	TOKEN_NAME,
	TOKEN_INT
};

struct token {
	enum token_kind kind;
	void *var;
};

struct token_list {
	unsigned int length;
	unsigned int index;
	struct token elems[];
};

void free_token(struct token*);
void free_token_list(struct token_list*);

/* ELEMENTS */

enum expr_kind {
	EXPR_INT,        /* var1: pointer to int */
	EXPR_NAME,       /* var1: pointer to char* */
	EXPR_CODE,       /* var1: pointer to function;
                        var2: pointer to unsigned char (nr. of arguments) */
	EXPR_LIST,       /* var1, var2: pointers to struct expression OR (nil) */
	EXPR_TUPLE,      /* var1, var2: pointers to struct expression */
	EXPR_APP         /* var1, var2: pointers to struct expression */
};

struct expression {
	enum expr_kind kind;
	void *var1;
	void *var2;
};

struct arg_list {
	struct expression elem;
	struct arg_list *rest;
};

struct rewrite_rule {
	char *name;
	struct arg_list *args;
	struct expression rhs;
};

struct fuspel {
	struct rewrite_rule rule;
	struct fuspel *rest;
};

bool empty_args_list(struct arg_list*);
unsigned char len_arg_list(struct arg_list*);

void cpy_expression(struct expression *dst, struct expression *src);
bool eq_expression(struct expression*, struct expression*);

void concat_fuspel(struct fuspel *start, struct fuspel *end);
struct fuspel *push_fuspel(struct fuspel*);
struct fuspel *pop_fuspel(struct fuspel*);
struct fuspel *popn_fuspel(struct fuspel*, unsigned char);

void free_expression(struct expression*);
void free_arg_list(struct arg_list*);
void free_rewrite_rule(struct rewrite_rule*);
void free_fuspel(struct fuspel*);

#endif
