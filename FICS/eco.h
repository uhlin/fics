#ifndef _ECO_H
#define _ECO_H

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

extern char	*boardToFEN();
extern char	*getECO();
extern int	 com_eco();
extern void	 BookInit();
extern void	 ECO_init();
extern void	 LONG_init();
extern void	 NIC_init();

#endif
