#ifndef _ECO_H
#define _ECO_H

#include "command.h" /* param_list */

#define SPACE_CHK()\
	do {\
		if (space > 0) {\
			FENstring[FENcount++] = (space + '0');\
			space = 0;\
		}\
	} while (0)

typedef struct {
	char	ECO[4];
	char	FENpos[80];
} ECO_entry;

typedef struct {
	char	NIC[6];
	char	FENpos[80];
} NIC_entry;

typedef struct {
	char	LONG[80];
	char	FENpos[80];
} LONG_entry;

extern char	*boardToFEN(int);
extern char	*getECO(int);
extern int	 com_eco(int, param_list);
extern void	 BookInit(void);
extern void	 ECO_init(void);
extern void	 LONG_init(void);
extern void	 NIC_init(void);

#endif
