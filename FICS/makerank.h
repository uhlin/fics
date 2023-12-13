#ifndef MAKERANK_H
#define MAKERANK_H

#include "config.h"

#define COMPUTER_FILE	DEFAULT_LISTS "/computer"
#define MAX_LOGIN_NAME	21

typedef struct _ratings {
	int	num;
	int	rating;
} ratings;

typedef struct _Entry {
	char	name[MAX_LOGIN_NAME];
	int	computer;
	ratings	r[4];
} ENTRY;

#endif
