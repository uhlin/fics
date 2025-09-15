#include <err.h>
#include <stdio.h>

#include "maxxes-utils.h"

int
main(void)
{
	FILE		*fp;
	char		 End[100] = { '\0' };
	char		 fmt[80] = { '\0' };
	const size_t	 End_size = sizeof End;
	int		 index = 0;
	long int	 when = 0;

	if ((fp = fopen("txt/stats-games.txt", "r")) == NULL)
		err(1, "fopen");

	msnprintf(fmt, sizeof fmt, "%%d %%*c %%*d %%*c %%*d %%*s %%*s %%*d "
	    "%%*d %%*d %%*d %%*s %%%zus %%ld\n", (End_size - 1));
	puts(fmt);

	do {
		if (fscanf(fp, fmt, &index, End, &when) != 3)
			warnx("items assigned mismatch");
		else {
			printf("index:\t%d\nEnd:\t%s\nwhen:\t%ld\n---\n",
			    index,
			    End,
			    when);
		}
	} while (!feof(fp) && !ferror(fp));

	fclose(fp);
	return 0;
}
