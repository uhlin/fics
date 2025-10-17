#ifndef COPYFILE_H
#define COPYFILE_H

#include <stdbool.h>

#include "common.h"

__FICS_BEGIN_DECLS
bool	fics_copyfile(const char *, const char *, const bool);
bool	is_regular_file(const char *);
__FICS_END_DECLS

#endif
