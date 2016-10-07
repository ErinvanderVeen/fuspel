#include "code.h"

#include <string.h>
#include <time.h>

#include "graphs.h"
#include "mem.h"
#include "print.h"

void fill_node_int(struct node** node, int i) {
	unsigned int used_count = (*node)->used_count;
	free_node(*node, used_count, 0);
	(*node)->kind = NODE_INT;
	(*node)->var1 = my_calloc(1, sizeof(int));
	*((int*) (*node)->var1) = i;
	use_node(*node, used_count);
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
	use_node(*node, used_count);
}

void code_time(struct node** result) {
	fill_node_int(result, (int) time(NULL));
}

void code_trace(struct node** result, struct node *p, struct node *r) {
	print_node(p);
	printf("\n");
	use_node(r, (*result)->used_count);
	free_node(*result, (*result)->used_count, 1);
	*result = r;
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

struct code_mapping {
	char* name;
	void* f;
	unsigned char arity;
};

static struct code_mapping code_table[] = {
	{ "time",  code_time,  0 },
	{ "trace", code_trace, 2 },
	{ "add",   code_add,   2 },
	{ "mul",   code_mul,   2 },
	{ "sub",   code_sub,   2 },
	{ "eq",    code_eq,    2 },
	{ "ge",    code_ge,    2 },
	{ "gt",    code_gt,    2 },
	{ "le",    code_le,    2 },
	{ "lt",    code_lt,    2 },
	{ "ne",    code_ne,    2 },
	{ NULL }
};

unsigned char code_find(char* name, void** function) {
	struct code_mapping *entry = code_table;
	while (entry) {
		if (!strcmp(name, entry->name)) {
			*function = entry->f;
			return entry->arity;
		}
		entry++;
	}

	*function = NULL;
	return 0;
}

#ifdef _FUSPEL_DEBUG
char *code_find_name(void* f) {
	struct code_mapping *entry = code_table;
	while (entry) {
		if (f == entry->f)
			return entry->name;
		entry++;
	}
	return NULL;
}
#endif
