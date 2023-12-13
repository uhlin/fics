#ifndef MAKERANK_H
#define MAKERANK_H

typedef struct _Entry {
	char	name[MAX_LOGIN_NAME];
	int	computer;
	ratings	r[4];
} ENTRY;

typedef struct _ratings {
	int	num;
	int	rating;
} ratings;

#endif
