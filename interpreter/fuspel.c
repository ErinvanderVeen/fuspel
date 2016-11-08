#include <stdio.h>
#include <string.h>

#ifdef FUSPEL_CLI
#include <argp.h>
#include <stdbool.h>
#endif

#include "eval.h"
#include "lex.h"
#include "mem.h"
#include "parse.h"
#include "print.h"

#define LINE_LENGTH 139

struct fuspel *import(struct fuspel *already_parsed, char *fname) {
	struct token_list *tokens = NULL;
	struct fuspel *pgm;
	FILE *f;
	char *fname_;

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

	if (!pgm) {
		fprintf(stderr, "Couldn't parse module %s.\n", fname);
		exit(EXIT_FAILURE);
	}

	concat_fuspel(pgm, already_parsed);

	return pgm;
}

#ifdef FUSPEL_CLI
const char *argp_prog_version = "fuspel";
const char *argp_prog_bugs = "<info@camilstaps.nl>";
static char doc[] = "Interpret a fuspel program";
static char args_doc[] = "MODULE [MODULE [MODULE [..]]]";
static struct argp_option options[] = {
	{ "print-program", 'P', 0, 0, "Print the parsed program before execution" },
#ifdef FUSPEL_DEBUG
	{ "debug-graphs", 'g', 0, 0, "Make a dot graph after every rewriting step" },
#endif // FUSPEL_DEBUG
	{ 0 }
};
struct environment {
	struct fuspel *program;
	bool printProgram;
#ifdef FUSPEL_DEBUG
	bool debugGraphs;
#endif // FUSPEL_DEBUG
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct environment *env = state->input;
	switch (key) {
		case 'P':
			env->printProgram = true;
			break;
#ifdef FUSPEL_DEBUG
		case 'g':
			env->debugGraphs = true;
			break;
#endif // FUSPEL_DEBUG
		case ARGP_KEY_ARG:
			env->program = import(env->program, arg);
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char *argv[]) {
	struct expression *result;
	struct environment env;

	env.printProgram = false;
	env.program = NULL;
#ifdef FUSPEL_DEBUG
	env.debugGraphs = false;
#endif

	argp_parse(&argp, argc, argv, 0, 0, &env);

	if (env.printProgram) {
		printf("\n");
		print_fuspel(env.program);
		printf("\n\n");
	}

#ifdef FUSPEL_DEBUG
	result = eval_main(env.program, env.debugGraphs);
#else
	result = eval_main(env.program);
#endif
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
#endif // FUSPEL_CLI
