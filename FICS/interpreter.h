#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <ctype.h> /* isspace */
#include <stdbool.h>
#include <stdio.h> /* FILE */
#include <string.h>

#include "common.h"

/*
 * Set to 0 to turn off this feature.
 */
#define IGNORE_UNRECOGNIZED_IDENTIFIERS 1

#define MAXLINE 3200

enum setting_type {
	TYPE_BOOLEAN,
	TYPE_INTEGER,
	TYPE_STRING
};

typedef bool (*Interpreter_vFunc)(const char *);
typedef int (*Interpreter_instFunc)(const char *, const char *);

struct Interpreter_in {
	char *path;
	char *line;
	long int line_num;
	Interpreter_vFunc validator_func;
	Interpreter_instFunc install_func;
};

__FICS_BEGIN_DECLS
extern const char g_fgets_nullret_err1[70];
extern const char g_fgets_nullret_err2[70];

void	Interpreter(const struct Interpreter_in *);
void	Interpreter_processAllLines(FILE *, const char *, Interpreter_vFunc,
	    Interpreter_instFunc);
__FICS_END_DECLS

static inline void
adv_while_isspace(const char **ptr)
{
	while (isspace(**ptr))
		(*ptr)++;
}

static inline bool
strings_match(const char *str1, const char *str2)
{
	return (strcmp(str1, str2) == 0);
}

#endif
