#include <stdio.h>
#include <string.h>

#ifdef _FUSPEL_CLI
#include <argp.h>
#include <stdbool.h>
#endif

#include "eval.h"
#include "lex.h"
#include "mem.h"
#include "parse.h"
#include "print.h"

#define LINE_LENGTH 139

fuspel* import(fuspel* already_parsed, char* fname) {
	token_list* tokens = NULL;
	fuspel* pgm;
	FILE* f;
	char* fname_;

	fname_ = my_calloc(1, strlen(fname) + 6);
	strcpy(fname_, fname);
	strcat(fname_, ".fusp");

	f = fopen(fname_, "r");
	if (!f) {
		fprintf(stderr, "Couldn't read %s.\n", fname_);
		exit(EXIT_FAILURE);
	}

	while (!feof(f)) {
		char program[LINE_LENGTH];
		if (!fgets(program, LINE_LENGTH, f)) {
			if (feof(f))
				break;
			fprintf(stderr, "Couldn't read %s.\n", fname_);
			exit(EXIT_FAILURE);
		}

		tokens = lex(tokens, program);
		if (!tokens) {
			fprintf(stderr, "Couldn't lex module %s.\n", fname);
			exit(EXIT_FAILURE);
		}
	}

	my_free(fname_);

	pgm = parse(tokens);
	free_token_list(tokens);
	my_free(tokens);

	concat_fuspel(pgm, already_parsed);

	return pgm;
}

#ifdef _FUSPEL_CLI
const char *argp_prog_version = "fuspel";
const char *argp_prog_bugs = "<info@camilstaps.nl>";
static char doc[] = "Interpret a fuspel program";
static char args_doc[] = "MODULE [MODULE [MODULE [..]]]";
static struct argp_option options[] = {
	{ "print-program", 'P', 0, 0, "Print the parsed program before execution" },
	{ 0 }
};
struct environment {
	fuspel* program;
	bool printProgram;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct environment *env = state->input;
	switch (key) {
		case 'P':
			env->printProgram = true;
			break;
		case ARGP_KEY_ARG:
			env->program = import(env->program, arg);
			return 0;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char* argv[]) {
	expression* result;
	struct environment env;

	env.printProgram = false;
	env.program = NULL;
	argp_parse(&argp, argc, argv, 0, 0, &env);
	
	if (!env.program) {
		fprintf(stderr, "Couldn't parse program.\n");
		exit(EXIT_FAILURE);
	}

	if (env.printProgram) {
		printf("\n");
		print_fuspel(env.program);
		printf("\n\n");
	}

	result = eval_main(env.program);
	if (result) {
		print_expression(result);
		printf("\n");

		free_expression(result);
		my_free(result);
	}

	free_fuspel(env.program);
	my_free(env.program);

	return 0;
}
#endif
