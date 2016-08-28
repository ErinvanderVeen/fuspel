#include <stdio.h>
#include <string.h>

#include "eval.h"
#include "lex.h"
#include "mem.h"
#include "parse.h"
#include "print.h"

#define LINE_LENGTH 139

fuspel* parse_file(fuspel* already_parsed, char* fname) {
	token_list* tokens = NULL;
	fuspel* pgm;
	FILE* f;
	char* fname_;

	fname_ = my_calloc(1, strlen(fname) + 6);
	strcpy(fname_, fname);
	strcat(fname_, ".fusp");

	f = fopen(fname_, "r");
	if (!f) {
		fprintf(stderr, "Couldn't read %s\n", fname_);
		exit(EXIT_FAILURE);
	}

	printf("Lexing %s...\n", fname_);

	while (!feof(f)) {
		char program[LINE_LENGTH];
		if (!fgets(program, LINE_LENGTH, f)) {
			if (feof(f))
				break;
			fprintf(stderr, "Couldn't read input.\n");
			exit(EXIT_FAILURE);
		}

		tokens = lex(tokens, program);
		if (!tokens) {
			fprintf(stderr, "Couldn't lex program.\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("Parsing %s...\n", fname_);

	my_free(fname_);

	pgm = parse(tokens);
	free_token_list(tokens);
	my_free(tokens);

	concat_fuspel(pgm, already_parsed);

	return pgm;
}

int main(int argc, char* argv[]) {
	expression* result;
	fuspel* pgm = NULL;
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s file [file [file [..]]]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (i = 1; i < argc; i++) {
		pgm = parse_file(pgm, argv[i]);
	}
	
	if (!pgm) {
		fprintf(stderr, "Couldn't parse program.\n");
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
