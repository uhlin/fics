/*
** Return codes from get_tcp_conn().
*/
#define FAIL		(-1)		/* routine failed */
#define	NOHOST		(FAIL-1)	/* no such host */
#define	NOSERVICE	(FAIL-2)	/* no such service */

extern int get_tcp_conn(char *, int);
extern unsigned long **name_to_address(char *);

/* extern int bcopy(); */
extern int socket();
extern int connect();
