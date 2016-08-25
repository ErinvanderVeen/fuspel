#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "lex.h"
#include "parse.h"
#include "eval.h"
#include "print.h"

static char* program =
	"singleton x = [x:[]];"
	"push x y = [x:y];"
	"is_zero 0 = 1;"
	"is_zero _ = 0;"
	"flip (a,b) = (b,a);"
	"main = push 3 (singleton (flip (is_zero 5, is_zero 0)));";
//static char* program =
//	"trip x y z = (x,(y,z));"
//	"main       = trip (trip 1 2 3) 10 (trip a b c);";
//static char* program =
//	"return   = (n,1);"
//	"main     = return;";

int main(void) {
	token_list* tokens = lex(program);
	
	if (!tokens)
		error(10, "Couldn't lex program.");
	
	printf("\n\nLexed program:\n");
	print_token_list(tokens);
	printf("\n");
	
	fuspel* pgm = parse(tokens);
	free_token_list(tokens);
	free(tokens);
	
	if (!pgm)
		error(10, "Couldn't parse program.");

	printf("\nParsed program:\n");
	print_fuspel(pgm);
	printf("\n\n\n");

	expression to_eval, *evaled;

	to_eval.kind = EXPR_NAME;
	to_eval.var1 = malloc(5);
	if (!to_eval.var1)
		error_no_mem();
	strcpy(to_eval.var1, "main");

	evaled = eval(pgm, &to_eval);
	if (evaled) {
		print_expression(evaled);
		printf("\n");

		free_expression(evaled);
		free(evaled);
	}

	return 0;
}
