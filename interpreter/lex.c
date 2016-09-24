#include "lex.h"

#include <string.h>

#include "mem.h"

inline unsigned is_space_char(char input) {
	return input == '\t' || input == ' ' || input == '\n' || input == '\r';
}

inline unsigned is_int_char(char input) {
	return '0' <= input && input <= '9';
}

/* The number of bytes that should be read to read an integer */
unsigned char lex_int_length(char* input) {
	unsigned char n = 0;
	while (is_int_char(*input++)) n++;
	return n;
}

inline unsigned is_name_char(char input) {
	return (('A' <= input && input <= 'Z') ||
			('a' <= input && input <= 'z') ||
			input == '_');
}

/* The number of bytes that should be read to read a name */
unsigned char lex_name_length(char* input) {
	unsigned char n = 0;
	while (is_name_char(*input++)) n++;
	return n;
}

token_list* lex(token_list* list, char* input) {
	token_list* first_list;
	unsigned create_new_token;

	while (*input && is_space_char(*input)) input++;
	if (*input == 0) {
		return list;
	}
	
	if (list) {
		first_list = list;
		while (list->rest) list = list->rest;
		list->rest = my_calloc(1, sizeof(token_list));
		list = list->rest;
	} else {
		first_list = list = my_calloc(1, sizeof(token_list));
	}

	create_new_token = 1;

	while (*input) {
		list->elem.var = NULL;

		switch (*input) {
			case ';': list->elem.kind = TOKEN_SEMICOLON; break;
			case ':': list->elem.kind = TOKEN_COLON;     break;
			case '(': list->elem.kind = TOKEN_OPEN_P;    break;
			case ')': list->elem.kind = TOKEN_CLOSE_P;   break;
			case '[': list->elem.kind = TOKEN_OPEN_SQ;   break;
			case ']': list->elem.kind = TOKEN_CLOSE_SQ;  break;
			case '=': list->elem.kind = TOKEN_EQUALS;    break;
			case ',': list->elem.kind = TOKEN_COMMA;     break;
			case '!': list->elem.kind = TOKEN_STRICT;    break;
			default:
				if (input[0] == '/' && input[1] == '/') {
					while (input && input[0] != '\n') input++;
					break;
				}
				if (input[0] == 'c' && input[1] == 'o' && input[2] == 'd' &&
						input[3] == 'e' && is_space_char(input[4])) {
					list->elem.kind = TOKEN_CODE;
					input += 4;
					break;
				}
				if (input[0] == 'i' && input[1] == 'm' && input[2] == 'p' &&
						input[3] == 'o' && input[4] == 'r' &&
						input[5] == 't' && is_space_char(input[6])) {
					list->elem.kind = TOKEN_IMPORT;
					input += 6;
					break;
				}
				if (is_int_char(*input)) {
					char* s;
					unsigned char len = lex_int_length(input);
					s = my_calloc(1, len + 1);
					list->elem.kind = TOKEN_INT;
					list->elem.var = my_calloc(1, sizeof(int));
					strncpy(s, input, len);
					*((int*) list->elem.var) = atoi(s);
					my_free(s);
					input += len - 1;
				} else if (is_name_char(*input)) {
					unsigned char len = lex_name_length(input);
					list->elem.kind = TOKEN_NAME;
					list->elem.var = my_calloc(1, len + 1);
					strncpy(list->elem.var, input, len);
					input += len - 1;
				} else if (is_space_char(*input)) {
					create_new_token = 0;
				} else {
					free_token_list(first_list);
					my_free(first_list);
					return NULL;
				}
		}

		do input++; while (*input && is_space_char(*input));

		if (*input && create_new_token) {
			list->rest = my_calloc(1, sizeof(token_list));
			list = list->rest;
		}
	}

	return first_list;
}
