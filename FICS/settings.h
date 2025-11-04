#ifndef SETTINGS_H
#define SETTINGS_H

#include "common.h"

typedef struct {
	char data[400];
} err_reason_t;

__FICS_BEGIN_DECLS
void		 settings_init(void);
void		 settings_deinit(void);

const char	*settings_get(const char *set_name);
void		 settings_read_conf(const char *path);
__FICS_END_DECLS

#endif
