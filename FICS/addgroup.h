#ifndef ADDGROUP_H
#define ADDGROUP_H

#include "common.h"

#include <stdbool.h>
#include <stdio.h>

__FICS_BEGIN_DECLS
int	fics_addgroup(const char *);
bool	get_group_id(const char *, int *);
bool	get_next_line_from_file(FILE *, char **); // uses 'new[]'
bool	group_exists(const char *);
bool	is_valid_group_name(const char *);
int	read_the_group_permissions_file(const char *);
__FICS_END_DECLS

#endif
