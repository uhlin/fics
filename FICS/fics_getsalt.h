#ifndef FICS_GETSALT_H
#define FICS_GETSALT_H

#define FICS_SALT_BEG	"$2b$10$"
#define FICS_SALT_SIZE	30

#ifdef __cplusplus
extern "C" {
#endif

char *fics_getsalt(void);

#ifdef __cplusplus
}
#endif

#endif
