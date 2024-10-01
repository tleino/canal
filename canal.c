/*
 * ISC License
 *
 * Copyright (c) 2021, Tommi Leino <namhas@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

enum {
	STATE_ANY, STATE_COMMENT_BEGIN, STATE_COMMENT_END,
	STATE_COMMENT_END_FIN, STATE_SKIP_EOL, STATE_IDENTIFIER,
	STATE_SQUOTE_END, STATE_DQUOTE_END
};

enum {
	/* Type specifiers */
	KW_STATIC, KW_CONST, KW_VOLATILE, KW_REGISTER, KW_EXTERN, KW_RESTRICT,
	KW_CHAR, KW_SHORT, KW_INT, KW_LONG, KW_FLOAT, KW_DOUBLE, KW_VOID,
	KW_UNSIGNED, KW_SIGNED, KW_SIZE_T, KW_SSIZE_T,
	KW_ENUM, KW_STRUCT, KW_UNION, KW_TYPEDEF,
	/* Constructs */
	KW_FOR, KW_SWITCH, KW_IF, KW_ELSE, KW_DO, KW_WHILE,
	/* Labels */
	KW_CASE,
	/* Construct controls */
	KW_BREAK, KW_CONTINUE,
	/* Function control */
	KW_RETURN, KW_GOTO,
	/* Built-in functions */
	KW_SIZEOF,
	/* Misc */
	KW_BOOL, KW_NULL
};

static const char *keyword[] = {
	"static", "const", "volatile", "register", "extern", "restrict",
	"char", "short", "int", "long", "float", "double", "void",
	"unsigned", "signed", "size_t", "ssize_t",
	"enum", "struct", "union", "typedef",
	"for", "switch", "if", "else", "do", "while",
	"case",
	"break", "continue",
	"return", "goto",
	"sizeof",
	"bool", "NULL"
};

static char **identifiers;
static size_t n_identifiers;
static size_t alloc_identifiers;
static size_t identifier_no;

static int state;
static int last_state;
static int escape;
static int col, line;
static int depth;

static char *follow;
static int follow_on = -1;

static char identifier[30];
static char finished_identifier[30];
static char *p;

static int is_keyword(size_t);
static size_t find_id(char *);
static const char *id_as_str(size_t);
static void check_follow(char *);

static size_t
find_id(char *identifier)
{
	size_t i;
	static const size_t kw_arr_len = sizeof(keyword) / sizeof(keyword[0]);

	for (i = 0; i < sizeof(keyword) / sizeof(keyword[0]); i++)
		if (strcmp(identifier, keyword[i]) == 0)
			return i;

	for (i = 0; i < n_identifiers; i++) {
		if (strcmp(identifier, identifiers[i]) == 0)
			return kw_arr_len + i; 
	}

	if (alloc_identifiers == n_identifiers) {
		if (alloc_identifiers == 0)
			alloc_identifiers = 16;
		else
			alloc_identifiers *= 2;
		identifiers = realloc(identifiers, sizeof(char *) *
		    alloc_identifiers);
	}

	identifiers[n_identifiers++] = strdup(identifier);
	return kw_arr_len + i;
}

static const char *
id_as_str(size_t idn)
{
	if (is_keyword(idn))
		return keyword[idn];

	return identifiers[idn - sizeof(keyword) / sizeof(keyword[0])];
}

static int
is_keyword(size_t idn)
{
	if (idn >= 0 && idn < sizeof(keyword) / sizeof(keyword[0]))
		return 1;

	return 0;
}

static int
usage(char *prog)
{
	fprintf(stderr,
	    "usage: %s\n"
	    "       %s follow FUNCTION\n", prog, prog);
	return 1;
}

static void
check_follow(char *identifier)
{
	if (follow != NULL) {
		if (follow_on == -1 && depth == 0 &&
		    strcmp(identifier, follow) == 0)
			follow_on = depth;
		else if (follow_on == depth &&
		    strcmp(identifier, follow) != 0)
			follow_on = -1;
	} else
		follow_on = 0;
}

int
main(int argc, char *argv[])
{
	char c;
	int i;

	if (argc >= 2) {
		if (strcmp(argv[1], "follow") == 0) {
			if (argc < 3)
				return usage(argv[0]);
			else
				follow = argv[2];
		} else
			return usage(argv[0]);
	}

	while ((c = getchar()) != EOF) {
		if (col == 0 && c == '#')
			state = STATE_SKIP_EOL;

		if (last_state == STATE_IDENTIFIER && state == STATE_ANY &&
			c == '(' && !is_keyword(identifier_no)) {
			printf("id '%s' is func...\n",
			    id_as_str(identifier_no));
		}

		last_state = state;
		switch (state) {
		case STATE_ANY:
			if (c == '/')
				state = STATE_COMMENT_BEGIN;
			else if ((c >= 'A' && c <= 'Z') ||
			    (c >= 'a' && c <= 'z') || c == '_') {
				p = &identifier[0];
				state = STATE_IDENTIFIER;
				*p++ = c;
			} else if (c == '\'') {
				state = STATE_SQUOTE_END;
			} else if (c == '\"') {
				state = STATE_DQUOTE_END;
			} else if (c == '{') {
				depth++;
			} else if (c == '}') {
				depth--;
			}
			break;
		case STATE_SQUOTE_END:
			if (c == '\\' && escape == 0)
				escape = 1;
			else if (c == '\'' && escape == 0)
				state = STATE_ANY;
			else
				escape = 0;
			break;
		case STATE_DQUOTE_END:
			if (c == '\\' && escape == 0)
				escape = 1;
			else if (c == '\"' && escape == 0)
				state = STATE_ANY;
			else
				escape = 0;
			break;
		case STATE_IDENTIFIER:
			if (!((c >= 'A' && c <= 'Z') ||
			    (c >= 'a' && c <= 'z') ||
			    (c >= 0 && c <= 9) || c == '_')) {
				*p = '\0';

				identifier_no = find_id(identifier);
#if 0
				if (!is_keyword(identifier_no)) {
					printf("<stdin>:%d: %s\n",
					    line + 1, identifier);
				}
#endif
				strcpy(finished_identifier, identifier);

				if (c == '(' && !is_keyword(identifier_no)) {
					check_follow(identifier);

					if (follow == NULL ||
					    (follow_on != -1 &&
					    follow_on != depth)) {
#if 1
						printf("%d\t", line);
						for (i = 0; i < depth; i++)
							putchar('\t');
#endif
						puts(id_as_str(identifier_no));
					}
				}
				state = STATE_ANY;
			} else
				*p++ = c;
			break;

		case STATE_COMMENT_BEGIN:
			if (c == '*')
				state = STATE_COMMENT_END;
			else
				state = STATE_ANY;
			break;
		case STATE_COMMENT_END:
			if (c == '*')
				state = STATE_COMMENT_END_FIN;
#if 0
			else
				printf("\033[31m%c\033[0m", c);
#endif
			break;
		case STATE_COMMENT_END_FIN:
			if (c == '/')
				state = STATE_ANY;
			else
				state = STATE_COMMENT_END;
			break;
		}

		if (c == '\n') {
			line++;
			col = 0;
			if (state == STATE_SKIP_EOL)
				state = STATE_ANY;
		} else
			col++;
	}

#if 0
	printf("n_identifiers: %d alloc_identifiers: %d (%lu bytes)\n",
	    n_identifiers,
	    alloc_identifiers, alloc_identifiers * sizeof(char *));
#endif

	return 0;
}
