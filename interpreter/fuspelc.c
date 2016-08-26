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
	expression* result;

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
	my_free(tokens);
	
	if (!pgm) {
		fprintf(stderr, "Couldn't parse program.");
		exit(EXIT_FAILURE);
	}

	printf("\n");
	print_fuspel(pgm);
	printf("\n\n");

	result = eval_main(pgm);
	if (result) {
		print_expression(result);
		printf("\n");

		free_expression(result);
		my_free(result);
	}

	free_fuspel(pgm);
	my_free(pgm);

	return 0;
}
