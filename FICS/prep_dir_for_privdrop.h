#ifndef PREP_DIR_FOR_PRIVDROP_H
#define PREP_DIR_FOR_PRIVDROP_H

#include "common.h"

__FICS_BEGIN_DECLS
int	drop_root_privileges(const char *);
int	prep_dir_for_privdrop(const char *);
__FICS_END_DECLS

#endif
