#include "lex.h"

#include <stdbool.h>
#include <string.h>

#include "mem.h"

static inline bool is_space_char(char input) {
	return input == '\t' || input == ' ' || input == '\n' || input == '\r';
}

static inline bool is_int_char(char input) {
	return '0' <= input && input <= '9';
}

/* The number of bytes that should be read to read an integer */
unsigned char lex_int_length(char *input) {
	unsigned char n = 0;
	while (is_int_char(*input++)) n++;
	return n;
}

static inline bool is_name_char(char input) {
	return (('A' <= input && input <= 'Z') ||
			('a' <= input && input <= 'z') ||
			input == '_');
}

/* The number of bytes that should be read to read a name */
unsigned char lex_name_length(char *input) {
	unsigned char n = 0;
	while (is_name_char(*input++)) n++;
	return n;
}

struct token_list *lex(struct token_list *list, char *input) {
	bool create_new_token;

	while (*input && is_space_char(*input)) input++;
	if (*input == 0)
		return list;
	
	if (!list) {
		list = my_calloc(1,
				sizeof(struct token_list) +
				INITIAL_TOKEN_LIST_SIZE * sizeof(struct token) + 1);
		list->length = INITIAL_TOKEN_LIST_SIZE;
		list->index = 0;
	}

	create_new_token = true;

	while (*input) {
		if (list->index >= list->length) {
			list = my_realloc(list,
					sizeof(struct token_list) +
					2 * list->length * sizeof(struct token) + 1);
			list->length *= 2;
		}

		list->elems[list->index].var = NULL;

		switch (*input) {
			case ';': list->elems[list->index].kind = TOKEN_SEMICOLON; break;
			case ':': list->elems[list->index].kind = TOKEN_COLON;     break;
			case '(': list->elems[list->index].kind = TOKEN_OPEN_P;    break;
			case ')': list->elems[list->index].kind = TOKEN_CLOSE_P;   break;
			case '[': list->elems[list->index].kind = TOKEN_OPEN_SQ;   break;
			case ']': list->elems[list->index].kind = TOKEN_CLOSE_SQ;  break;
			case '=': list->elems[list->index].kind = TOKEN_EQUALS;    break;
			case ',': list->elems[list->index].kind = TOKEN_COMMA;     break;
			default:
				if (input[0] == '/' && input[1] == '/') {
					while (input && input[0] != '\n') input++;
					create_new_token = false;
					break;
				} else if (input[0] == 'c' && input[1] == 'o' &&
						input[2] == 'd' && input[3] == 'e' &&
						is_space_char(input[4])) {
					list->elems[list->index].kind = TOKEN_CODE;
					input += 4;
					break;
				} else if (input[0] == 'i' && input[1] == 'm' &&
						input[2] == 'p' && input[3] == 'o' &&
						input[4] == 'r' && input[5] == 't' &&
						is_space_char(input[6])) {
					list->elems[list->index].kind = TOKEN_IMPORT;
					input += 6;
					break;
				} else if (is_int_char(*input)) {
					char *s;
					unsigned char len = lex_int_length(input);
					s = my_calloc(1, len + 1);
					strncpy(s, input, len);
					list->elems[list->index].kind = TOKEN_INT;
					list->elems[list->index].var = (void*)((INT)atoi(s));
					my_free(s);
					input += len - 1;
				} else if (is_name_char(*input)) {
					unsigned char len = lex_name_length(input);
					list->elems[list->index].kind = TOKEN_NAME;
					list->elems[list->index].var = my_calloc(1, len + 1);
					strncpy(list->elems[list->index].var, input, len);
					input += len - 1;
				} else if (is_space_char(*input)) {
					create_new_token = false;
				} else {
					free_token_list(list);
					my_free(list);
					return NULL;
				}
		}

		do input++; while (*input && is_space_char(*input));

		if (create_new_token)
			list->index++;
	}

	return list;
}
