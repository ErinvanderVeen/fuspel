#ifndef _H_SYNTAX
#define _H_SYNTAX

#include <stdbool.h>

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
	EXPR_INT,        /* var1: pointer to int */
	EXPR_NAME,       /* var1: pointer to char* */
	EXPR_CODE,       /* var1: pointer to function;
                        var2: pointer to unsigned char (nr. of arguments) */
	EXPR_LIST,       /* var1, var2: pointers to expression OR (nil) */
	EXPR_TUPLE,      /* var1, var2: pointers to expression */
	EXPR_APP         /* var1, var2: pointers to expression */
} expr_kind;

typedef struct {
	expr_kind kind;
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

bool empty_args_list(arg_list*);
unsigned char len_arg_list(arg_list*);

void cpy_expression(expression* dst, expression* src);
bool eq_expression(expression*, expression*);

void concat_fuspel(fuspel* start, fuspel* end);
fuspel* push_fuspel(fuspel*);
fuspel* pop_fuspel(fuspel*);
fuspel* popn_fuspel(fuspel*, unsigned char);

void free_expression(expression*);
void free_arg_list(arg_list*);
void free_rewrite_rule(rewrite_rule*);
void free_fuspel(fuspel*);

#endif
