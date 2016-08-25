#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "lex.h"
#include "parse.h"
#include "eval.h"
#include "print.h"

int main(void) {
	token_list* tokens = NULL;

	while (!feof(stdin)) {
		char program[79];
		if (!fgets(program, 79, stdin)) {
			if (feof(stdin))
				break;
			error(11, "Couldn't read input.");
		}

		tokens = lex(tokens, program);
		if (!tokens)
			error(12, "Couldn't lex program.");
	}
	
	fuspel* pgm = parse(tokens);
	free_token_list(tokens);
	free(tokens);
	
	if (!pgm)
		error(13, "Couldn't parse program.");

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
	free_expression(&to_eval);
	if (evaled) {
		print_expression(evaled);
		printf("\n");

		free_expression(evaled);
		free(evaled);
	}

	free_fuspel(pgm);
	free(pgm);

	return 0;
}
