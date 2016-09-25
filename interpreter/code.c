#include "code.h"

#include <string.h>
#include <time.h>

#include "mem.h"

void fill_node_int(struct node** node, int i) {
	unsigned int used_count = (*node)->used_count;
	free_node(*node, used_count, 0);
	(*node)->kind = NODE_INT;
	(*node)->var1 = my_calloc(1, sizeof(int));
	*((int*) (*node)->var1) = i;
	use_node(*node, used_count - 1);
}

void fill_node_bool(struct node** node, int i) {
	fill_node_int(node, i ? 1 : 0);
}

void fill_node_name(struct node** node, char* s) {
	unsigned int used_count = (*node)->used_count;
	free_node(*node, used_count, 0);
	(*node)->kind = NODE_NAME;
	(*node)->var1 = my_calloc(1, strlen(s) + 1);
	strcpy((*node)->var1, s);
	use_node(*node, used_count - 1);
}

void code_time(struct node** result) {
	fill_node_int(result, (int) time(NULL));
}

void code_add(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "add on non-ints");
	else
		fill_node_int(result, *((int*) b->var1) + *((int*) a->var1));
}

void code_mul(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "mul on non-ints");
	else
		fill_node_int(result, *((int*) a->var1) * *((int*) b->var1));
}

void code_sub(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "sub on non-ints");
	else
		fill_node_int(result, *((int*) b->var1) - *((int*) a->var1));
}

void code_eq(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "eq on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) == *((int*) b->var1));
}

void code_gt(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "gt on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) > *((int*) b->var1));
}

void code_ge(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "ge on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) >= *((int*) b->var1));
}

void code_lt(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "lt on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) < *((int*) b->var1));
}

void code_le(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "le on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) <= *((int*) b->var1));
}

void code_ne(struct node** result, struct node* a, struct node* b) {
	if (a->kind != NODE_INT || b->kind != NODE_INT)
		fill_node_name(result, "ne on non-ints");
	else
		fill_node_bool(result, *((int*) a->var1) != *((int*) b->var1));
}

unsigned char code_find(char* name, void** function) {
	if (!strcmp(name, "time")) {
		*function = (void(*)(void)) code_time;
		return 0;
	} else if (!strcmp(name, "add")) {
		*function = (void(*)(void)) code_add;
		return 2;
	} else if (!strcmp(name, "mul")) {
		*function = (void(*)(void)) code_mul;
		return 2;
	} else if (!strcmp(name, "sub")) {
		*function = (void(*)(void)) code_sub;
		return 2;
	} else if (!strcmp(name, "eq")) {
		*function = (void(*)(void)) code_eq;
		return 2;
	} else if (!strcmp(name, "ge")) {
		*function = (void(*)(void)) code_ge;
		return 2;
	} else if (!strcmp(name, "gt")) {
		*function = (void(*)(void)) code_gt;
		return 2;
	} else if (!strcmp(name, "le")) {
		*function = (void(*)(void)) code_le;
		return 2;
	} else if (!strcmp(name, "lt")) {
		*function = (void(*)(void)) code_lt;
		return 2;
	} else if (!strcmp(name, "ne")) {
		*function = (void(*)(void)) code_ne;
		return 2;
	}

	*function = NULL;
	return 0;
}
