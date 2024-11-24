/*
 * Revised by maxxe 23/12/14
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __linux__
#include <bsd/string.h>
#endif

#include "common.h"
#include "makerank.h"

static ENTRY	**list;
static ENTRY	**sortme;

static char	*rnames[] = { "std", "blitz", "wild", "lightning" };
static int	 rtype;

static int
GetPlayerInfo(char *fileName, ENTRY *e)
{
	FILE	*fp;
	char	 NameWithCase[30];
	char	 field[20];
	char	 line[100];
	int	 done = 0;

	e->computer = 0;

	for (size_t i = 0; i < ARRAY_SIZE(e->r); i++) {
		e->r[i].num	= 0;
		e->r[i].rating	= 0;
	}

	if ((fp = fopen(fileName, "r")) == NULL ||
	    fgets(line, sizeof line - 1, fp) == NULL ||
	    feof(fp))
		return 0;

	if (!strcmp(line, "v 1\n")) {
		fgets(line, 99, fp);

		sscanf(line, "%s", e->name);

		fgets(line, 99, fp);
		fgets(line, 99, fp);
		fgets(line, 99, fp);

		if (fscanf(fp, "%d %*u %*u %*u %d %*u %*u %*u %*u %d %*u %*u "
		    "%*u %d %*u %*u %*u %*u %d %*u %*u %*u %d %*u %*u %*u %*u "
		    "%d %*u %*u %*u %d %*u %*u %*u %*u",
		    &(e->r[0].num),
		    &(e->r[0].rating),
		    &(e->r[1].num),
		    &(e->r[1].rating),
		    &(e->r[2].num),
		    &(e->r[2].rating),
		    &(e->r[3].num),
		    &(e->r[3].rating)) != 8) {
			fprintf(stderr, "OOPS: couldn't parse player file %s."
			    "\n", fileName);
		}
	} else {
		do {
			sscanf(line, "%s", field);

			if (!strcmp(field, "Name:")) {
				sscanf(line, "%*s %s", NameWithCase);

				if (strcasecmp(e->name, NameWithCase)) {
					printf("TROUBLE: %s's handle is "
					    "listed as %s.\n", e->name,
					    NameWithCase);
				} else if (strlcpy(e->name, NameWithCase,
				    sizeof e->name) >= sizeof e->name) {
					fprintf(stderr, "%s: warning: "
					    "strlcpy() truncated\n", __func__);
				}
			} else if (!strcmp(field, "S_NUM:")) {
				sscanf(line, "%*s %d", &(e->r[0].num));
			} else if (!strcmp(field, "B_NUM:")) {
				sscanf(line, "%*s %d", &(e->r[1].num));
			} else if (!strcmp(field, "W_NUM:")) {
				sscanf(line, "%*s %d", &(e->r[2].num));
			} else if (!strcmp(field, "L_NUM:")) {
				sscanf(line, "%*s %d", &(e->r[3].num));
			} else if (!strcmp(field, "S_RATING:")) {
				sscanf(line, "%*s %d", &(e->r[0].rating));
			} else if (!strcmp(field, "B_RATING:")) {
				sscanf(line, "%*s %d", &(e->r[1].rating));
			} else if (!strcmp(field, "W_RATING:")) {
				sscanf(line, "%*s %d", &(e->r[2].rating));
			} else if (!strcmp(field, "L_RATING:")) {
				sscanf(line, "%*s %d", &(e->r[3].rating));
			} else if (!strcmp(field, "Network:")) {
				done = 1;
			}

			fgets(line, 99, fp);
		} while (!done && !feof(fp));
	}

	fclose(fp);
	return 1;
}

static int
LoadEntries(void)
{
	ENTRY	 e;
	FILE	*fpPlayerList;
	char	 command[90];
	char	 letter1;
	char	 pathInput[80];
	int	 len, n = 0;
	int	 listsize;

	listsize	= 100;
	list		= reallocarray(NULL, sizeof(ENTRY *), listsize);

	if (list == NULL)
		err(1, "%s: reallocarray", __func__);

	for (letter1 = 'a'; letter1 <= 'z'; letter1++) {
		printf("Loading %c's.\n", letter1);

		snprintf(pathInput, sizeof pathInput, "%s/%c", DEFAULT_PLAYERS,
		    letter1);
		snprintf(command, sizeof command, "ls -1 %s", pathInput);

		if ((fpPlayerList = popen(command, "r")) == NULL)
			continue;

		while (1) {
			fgets(e.name, MAX_LOGIN_NAME, fpPlayerList);

			if (feof(fpPlayerList))
				break;

			len = strlen(e.name);
			e.name[len - 1] = '\0';

			if (e.name[0] != letter1) {
				printf("File %c/%s: wrong directory.\n",
				    letter1, e.name);
			} else {
				snprintf(pathInput, sizeof pathInput,
				    "%s/%c/%s", DEFAULT_PLAYERS, letter1,
				    e.name);

				if (GetPlayerInfo(pathInput, &e)) {
					if ((list[n] = malloc(sizeof(ENTRY))) ==
					    NULL)
						err(1, "%s: malloc", __func__);

					memcpy(list[n], &e, sizeof(ENTRY));

					if (++n == listsize) {
						listsize += 100;
						list = reallocarray(list,
						    listsize,
						    sizeof(ENTRY *));
						if (list == NULL)
							err(1, NULL);
					}
				}
			}
		} /* while (1) */

		pclose(fpPlayerList);
	} /* for */

	return n;
}

static int
SetComputers(int n)
{
	FILE	*fpComp;
	char	 comp[30];
	char	 line[100];
	int	 i = 0;

	if (snprintf(line, sizeof line, "sort -f %s", COMPUTER_FILE) >=
	    (int)sizeof line) {
		warnx("%s: snprintf truncated", __func__);
		return 0;
	} else if ((fpComp = popen(line, "r")) == NULL)
		return 0;

	while (i < n) {
		fgets(comp, 29, fpComp);

		if (feof(fpComp))
			break;

		comp[strlen(comp) - 1] = '\0';

		while (i < n && strcasecmp(list[i]->name, comp) < 0)
			i++;
		if (i < n && strcasecmp(list[i]->name, comp) == 0)
			list[i++]->computer = 1;
	}

	pclose(fpComp);
	return 1;
}

static int
sortfunc(const void *i, const void *j)
{
	int n =	(*(ENTRY **)j)->r[rtype].rating -
		(*(ENTRY **)i)->r[rtype].rating;

	return n ? n : strcasecmp((*(ENTRY **)i)->name, (*(ENTRY **)j)->name);
}

static void
makerank(void)
{
	FILE	*fp;
	char	 fName[200];
	int	 sortnum, sortmesize, i, n;

	printf("Loading players\n");
	n = LoadEntries();
	printf("Found %d players.\n", n);
	printf("Setting computers.\n");
	SetComputers(n);

	for (rtype = 0; rtype < 4; rtype++) {
		sortnum		= 0;
		sortmesize	= 100;
		sortme		= reallocarray(NULL, sizeof(ENTRY *),
		    sortmesize);

		if (sortme == NULL)
			err(1, "%s: reallocarray", __func__);

		for (i = 0; i < n; i++) {
			if (list[i]->r[rtype].rating) {
				sortme[sortnum++] = list[i];

				if (sortnum == sortmesize) {
					sortmesize += 100;
					sortme = reallocarray(sortme,
					    sortmesize,
					    sizeof(ENTRY *));
					if (sortme == NULL)
						err(1, NULL);
				}
			}
		}

		printf("Sorting %d %s.\n", sortnum, rnames[rtype]);
		qsort(sortme, sortnum, sizeof(ENTRY *), sortfunc);

		printf("Saving to file.\n");
		snprintf(fName, sizeof fName, "%s/rank.%s", DEFAULT_STATS,
		    rnames[rtype]);

		if ((fp = fopen(fName, "w")) == NULL)
			err(1, "%s: fopen", __func__);

		for (i = 0; i < sortnum; i++) {
			fprintf(fp, "%s %d %d %d\n",
			    sortme[i]->name,
			    sortme[i]->r[rtype].rating,
			    sortme[i]->r[rtype].num,
			    sortme[i]->computer);
		}
		fclose(fp);
		free(sortme);
	}
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		fprintf(stderr, "usage: %s.\n", argv[0]);
		return EXIT_FAILURE;
	}

	makerank();
	return EXIT_SUCCESS;
}
