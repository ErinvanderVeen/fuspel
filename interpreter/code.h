#ifndef _H_CODE
#define _H_CODE

#include "syntax.h"
#include "graphs.h"

typedef void (Code_0) (struct node**);
typedef void (Code_1) (struct node**, struct node*);
typedef void (Code_2) (struct node**, struct node*, struct node*);

unsigned char code_find(char* name, void** function);

void code_mul(struct node** result, struct node* a, struct node* b);
void code_sub(struct node** result, struct node* a, struct node* b);

#endif
