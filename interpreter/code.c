#include "code.h"

#include <string.h>
#include <time.h>

#include "mem.h"

expression* make_int_expression(int i) {
	expression* result = my_calloc(1, sizeof(expression));
	result->kind = EXPR_INT;
	result->var1 = my_calloc(1, sizeof(int));
	*((int*) result->var1) = i;
	return result;
}

expression* make_name_expression(char* s) {
	expression* result = my_calloc(1, sizeof(expression));
	result->kind = EXPR_NAME;
	result->var1 = my_calloc(1, strlen(s) + 1);
	strcpy(result->var1, s);
	return result;
}

expression* code_time(void) {
	return make_int_expression((int) time(NULL));
}

expression* code_mul(expression* a, expression* b) {
	return (a->kind != EXPR_INT || b->kind != EXPR_INT)
		? make_name_expression("mul on non-ints")
		: make_int_expression(*((int*) a->var1) * *((int*) b->var1));
}

expression* code_sub(expression* a, expression* b) {
	return (a->kind != EXPR_INT || b->kind != EXPR_INT)
		? make_name_expression("sub on non-ints")
		: make_int_expression(*((int*) b->var1) - *((int*) a->var1));
}

unsigned char code_find(char* name, void** function) {
	if (!strcmp(name, "mul")) {
		*function = (void(*)(void)) code_mul;
		return 2;
	} else if (!strcmp(name, "sub")) {
		*function = (void(*)(void)) code_sub;
		return 2;
	} else if (!strcmp(name, "time")) {
		*function = (void(*)(void)) code_time;
		return 0;
	}

	*function = NULL;
	return 0;
}
