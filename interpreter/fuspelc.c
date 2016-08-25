#include <stdio.h>
#include <string.h>

#include "eval.h"
#include "lex.h"
#include "mem.h"
#include "parse.h"
#include "print.h"

int main(void) {
	token_list* tokens;
	fuspel* pgm;
	expression to_eval, *evaled;

	tokens = NULL;

	while (!feof(stdin)) {
		char program[79];
		if (!fgets(program, 79, stdin)) {
			if (feof(stdin))
				break;
			fprintf(stderr, "Couldn't read input.");
			exit(EXIT_FAILURE);
		}

		tokens = lex(tokens, program);
		if (!tokens) {
			fprintf(stderr, "Couldn't lex program.");
			exit(EXIT_FAILURE);
		}
	}
	
	pgm = parse(tokens);
	free_token_list(tokens);
	free(tokens);
	
	if (!pgm) {
		fprintf(stderr, "Couldn't parse program.");
		exit(EXIT_FAILURE);
	}

	printf("\nParsed program:\n");
	print_fuspel(pgm);
	printf("\n\n\n");

	to_eval.kind = EXPR_NAME;
	to_eval.var1 = my_calloc(1, 5);
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
