#ifndef FICS_GETSALT_H
#define FICS_GETSALT_H

#include "common.h"

#define FICS_SALT_BEG	"$2b$10$"
#define FICS_SALT_SIZE	30

__FICS_BEGIN_DECLS
char *fics_getsalt(void);
__FICS_END_DECLS

#endif
