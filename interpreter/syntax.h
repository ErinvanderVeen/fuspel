#ifndef _H_SYNTAX
#define _H_SYNTAX

/* TOKENS */

typedef enum {
	TOKEN_SEMICOLON, /* ; */
	TOKEN_EQUALS,    /* = */
	TOKEN_OPEN_SQ,   /* [ */
	TOKEN_CLOSE_SQ,  /* ] */
	TOKEN_OPEN_P,    /* ( */
	TOKEN_CLOSE_P,   /* ) */
	TOKEN_COMMA,     /* , */
	TOKEN_COLON,     /* : */
	TOKEN_STRICT,    /* ! */
	TOKEN_CODE,      /* code */
	TOKEN_IMPORT,    /* import */
	TOKEN_NAME,
	TOKEN_INT
} token_kind;

typedef struct {
	token_kind kind;
	void* var;
} token;

typedef struct token_list {
	token elem;
	struct token_list* rest;
} token_list;

void free_token(token*);
void free_token_list(token_list*);

/* ELEMENTS */

typedef enum {
	EXPR_INT,
	EXPR_NAME,
	EXPR_CODE,
	EXPR_LIST,
	EXPR_TUPLE,
	EXPR_APP
} expr_kind;

typedef struct {
	expr_kind kind;
	unsigned is_strict;
	void* var1;
	void* var2;
} expression;

typedef struct arg_list {
	expression elem;
	struct arg_list* rest;
} arg_list;

typedef struct {
	char* name;
	arg_list* args;
	expression rhs;
} rewrite_rule;

typedef struct fuspel {
	rewrite_rule rule;
	struct fuspel* rest;
} fuspel;

unsigned empty_args_list(arg_list*);
unsigned char len_arg_list(arg_list*);

void cpy_expression(expression* dst, expression* src);
unsigned eq_expression(expression*, expression*);
expression** flatten_app_args(expression*);

void concat_fuspel(fuspel* start, fuspel* end);
fuspel* push_fuspel(fuspel*);
fuspel* pop_fuspel(fuspel*);
fuspel* popn_fuspel(fuspel*, unsigned char);

void free_expression(expression*);
void free_arg_list(arg_list*);
void free_rewrite_rule(rewrite_rule*);
void free_fuspel(fuspel*);

#endif
