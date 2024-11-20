/* eco.c
 *
 */

#include "stdinclude.h"
#include "common.h"

#include <err.h>

#include "board.h"
#include "command.h"
#include "config.h"
#include "eco.h"
#include "gamedb.h"
#include "gameproc.h"
#include "obsproc.h"
#include "playerdb.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

#define FENPOS_SIZE 73
#define ONMOVE_SIZE 2

#define ECO_MAXFILENAME 1024
#define ECO_MAXTMP 1024

#define SCAN_FP_AND_ONMOVE "%72[\x21-z] %1s"

PRIVATE char *book_dir = DEFAULT_BOOK;

PRIVATE ECO_entry	*ECO_book[1096];
PRIVATE NIC_entry	*NIC_book[1096];
PRIVATE LONG_entry	*LONG_book[4096];

PRIVATE int ECO_entries, NIC_entries, LONG_entries;

PRIVATE inline int
fencmp(const unsigned char *pos1, const char *pos2)
{
	return strcmp((const char *)pos1, pos2);
}

PUBLIC char *
boardToFEN(int g)
{
	int		FENcount = 0;
	int		space = 0;
	static char	FENstring[80];

	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			switch (garray[g].game_state.board[j][i]) {
			case W_PAWN:
				SPACE_CHK();
				FENstring[FENcount++] = 'P';
				break;
			case W_ROOK:
				SPACE_CHK();
				FENstring[FENcount++] = 'R';
				break;
			case W_KNIGHT:
				SPACE_CHK();
				FENstring[FENcount++] = 'N';
				break;
			case W_BISHOP:
				SPACE_CHK();
				FENstring[FENcount++] = 'B';
				break;
			case W_QUEEN:
				SPACE_CHK();
				FENstring[FENcount++] = 'Q';
				break;
			case W_KING:
				SPACE_CHK();
				FENstring[FENcount++] = 'K';
				break;
			case B_PAWN:
				SPACE_CHK();
				FENstring[FENcount++] = 'p';
				break;
			case B_ROOK:
				SPACE_CHK();
				FENstring[FENcount++] = 'r';
				break;
			case B_KNIGHT:
				SPACE_CHK();
				FENstring[FENcount++] = 'n';
				break;
			case B_BISHOP:
				SPACE_CHK();
				FENstring[FENcount++] = 'b';
				break;
			case B_QUEEN:
				SPACE_CHK();
				FENstring[FENcount++] = 'q';
				break;
			case B_KING:
				SPACE_CHK();
				FENstring[FENcount++] = 'k';
				break;
			default:
				space++;
				break;
			} /* switch */
		} /* for */

		if (space > 0) {
			FENstring[FENcount++] = (space + '0');
			space = 0;
		}

		FENstring[FENcount++] = '/';
	} /* for */

	FENstring[--FENcount] = ' ';
	FENstring[++FENcount] = ((garray[g].game_state.onMove == WHITE)
				 ? 'w'
				 : 'b');
	FENstring[++FENcount] = '\0';

	return FENstring;
}

PUBLIC void
ECO_init(void)
{
	FILE	*fp;
	char	 ECO[4] = {0,0,0,0};
	char	 FENpos[FENPOS_SIZE] = { '\0' };
	char	 filename[ECO_MAXFILENAME] = { '\0' };
	char	 onMove[ONMOVE_SIZE] = {0,0};
	char	 tmp[ECO_MAXTMP] = { '\0' };
	char	*ptmp = tmp;
	int	 i = 0;

	snprintf(filename, sizeof filename, "%s/eco999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL)
		err(1, "Could not open ECO file (%s)", filename);

	while (!feof(fp)) {
		(void) strlcpy(ptmp, "", sizeof tmp);

		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;

		/* XXX */
		sscanf(ptmp, SCAN_FP_AND_ONMOVE, FENpos, onMove);
		(void) strlcat(FENpos, " ", sizeof FENpos);
		(void) strlcat(FENpos, onMove, sizeof FENpos);

		(void) strlcpy(ptmp, "", sizeof tmp);
		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;
		sscanf(ptmp, "%[0-z]", ECO);

		if ((ECO_book[i] = malloc(sizeof(ECO_entry))) == NULL)
			err(1, "Cound not alloc mem for ECO entry %d", i);

		(void) strlcpy(ECO_book[i]->ECO, ECO,
		    sizeof(ECO_book[i]->ECO));
		(void) strlcpy(ECO_book[i]->FENpos, FENpos,
		    sizeof(ECO_book[i]->FENpos));

		++i;
	}

	fclose(fp);
	ECO_book[i] = NULL;

	fprintf(stderr, "%d entries in ECO book\n", i);
	ECO_entries = i;

	while (--i >= 0) {
		if (ECO_book[i] == NULL) {
			fprintf(stderr, "ERROR! ECO book position number %d "
			    "is NULL.", i);
		}
	}
}

PUBLIC void
NIC_init(void)
{
	FILE	*fp;
	char	 FENpos[FENPOS_SIZE] = { '\0' };
	char	 NIC[6] = {0,0,0,0,0,0};
	char	 filename[ECO_MAXFILENAME] = { '\0' };
	char	 onMove[ONMOVE_SIZE] = {0,0};
	char	 tmp[ECO_MAXTMP] = { '\0' };
	char	*ptmp = tmp;
	int	 i = 0;

	snprintf(filename, sizeof filename, "%s/nic999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL)
		err(1, "Could not open NIC file (%s)", filename);

	while (!feof(fp)) {
		(void) strlcpy(ptmp, "", sizeof tmp);

		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;

		sscanf(ptmp, SCAN_FP_AND_ONMOVE, FENpos, onMove);
		(void) strlcat(FENpos, " ", sizeof FENpos);
		(void) strlcat(FENpos, onMove, sizeof FENpos);

		(void) strlcpy(ptmp, "", sizeof tmp);
		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;
		sscanf(ptmp, "%[.-z]", NIC);

		if ((NIC_book[i] = malloc(sizeof(NIC_entry))) == NULL)
			err(1, "Cound not alloc mem for NIC entry %d", i);

		(void) strlcpy(NIC_book[i]->NIC, NIC,
		    sizeof(NIC_book[i]->NIC));
		(void) strlcpy(NIC_book[i]->FENpos, FENpos,
		    sizeof(NIC_book[i]->FENpos));

		++i;
	}

	fclose(fp);
	NIC_book[i] = NULL;

	fprintf(stderr, "%d entries in NIC book\n", i);
	NIC_entries = i;
}

PUBLIC void
LONG_init(void)
{
	FILE	*fp;
	char	 FENpos[FENPOS_SIZE] = { '\0' };
	char	 LONG[256] = { '\0' };
	char	 filename[ECO_MAXFILENAME] = { '\0' };
	char	 onMove[ONMOVE_SIZE] = {0,0};
	char	 tmp[ECO_MAXTMP] = { '\0' };
	char	*ptmp = tmp;
	int	 i = 0;

	snprintf(filename, sizeof filename, "%s/long999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL)
		err(1, "Could not open LONG file (%s)", filename);

	while (!feof(fp)) {
		(void) strlcpy(ptmp, "", sizeof tmp);

		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;

		/* XXX */
		sscanf(ptmp, SCAN_FP_AND_ONMOVE, FENpos, onMove);
		(void) strlcat(FENpos, " ", sizeof FENpos);
		(void) strlcat(FENpos, onMove, sizeof FENpos);

		(void) strlcpy(ptmp, "", sizeof tmp);
		if (fgets(ptmp, sizeof tmp, fp) == NULL ||
		    feof(fp))
			break;
		sscanf(ptmp, "%[^*\n]", LONG);

		if ((LONG_book[i] = malloc(sizeof(LONG_entry))) == NULL)
			err(1, "Cound not alloc mem for LONG entry %d", i);

		(void) strlcpy(LONG_book[i]->LONG, LONG,
		    sizeof(LONG_book[i]->LONG));
		(void) strlcpy(LONG_book[i]->FENpos, FENpos,
		    sizeof(LONG_book[i]->FENpos));

		++i;
	}

	fclose(fp);
	LONG_book[i] = NULL;

	fprintf(stderr, "%d entries in LONG book\n", i);
	LONG_entries = i;
}

PUBLIC void
BookInit(void)
{
	ECO_init();
	NIC_init();
	LONG_init();
}

PUBLIC char *
getECO(int g)
{
	static char ECO[4];

#ifndef IGNORE_ECO
	int i, flag, l, r, x;

	if (parray[garray[g].white].private ||
	    parray[garray[g].black].private) {
		(void) strlcpy(ECO, "---", sizeof ECO);
		return ECO;
	} else {
		if (garray[g].type == TYPE_WILD) {
			(void) strlcpy(ECO, "---", sizeof ECO);
			return ECO;
		} else if (garray[g].moveList == NULL) {
			(void) strlcpy(ECO, "***", sizeof ECO);
			return ECO;
		} else {
			(void) strlcpy(ECO, "A00", sizeof ECO);
		}
	}

	flag	= 0;
	i	= garray[g].numHalfMoves;

	while (i > 0 && !flag) {
		l = 0;
		r = (ECO_entries - 1);

		while ((r >= l) && !flag) {
			x = ((l + r) / 2);

			if (fencmp(garray[g].moveList[i].FENpos,
			    ECO_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!fencmp(garray[g].moveList[i].FENpos,
			    ECO_book[x]->FENpos)) {
				(void)strlcpy(ECO, ECO_book[x]->ECO,
				    sizeof ECO);
				flag = 1;
			}
		}

		i--;
	} /* while */
#else
	(void) strlcpy(ECO, "---", sizeof ECO);
#endif
	return ECO;
}

PUBLIC int
com_eco(int p, param_list param)
{
#ifndef IGNORE_ECO
	int	g1, p1;
	int	i, flag = 0, x, l, r;

	if (param[0].type == TYPE_NULL) {    // own game
		if (parray[p].game < 0) {
			pprintf(p, "You are not playing or examining a game."
			    "\n");
			return COM_OK;
		}

		g1 = parray[p].game;

		if (garray[g1].status != GAME_EXAMINE && !pIsPlaying(p))
			return COM_OK;
	} else {
		if ((g1 = GameNumFromParam(p, &p1, &param[0])) < 0)
			return COM_OK;
		if (g1 >= g_num ||
		    (garray[g1].status != GAME_ACTIVE &&
		    garray[g1].status != GAME_EXAMINE)) {
			pprintf(p, "There is no such game.\n");
			return COM_OK;
		}
	}

	if ((parray[garray[g1].white].private ||
	    parray[garray[g1].black].private) &&
	    parray[p].adminLevel == 0) {
		pprintf(p, "Sorry - that game is private.\n");
		return COM_OK;
	} else {
		if (garray[g1].type == TYPE_WILD) {
			pprintf(p, "That game is a wild game.\n");
			return COM_OK;
		}
	}

	pprintf(p, "Info about game %d: \"%s vs. %s\"\n\n", (g1 + 1),
	    garray[g1].white_name,
	    garray[g1].black_name);

	if (garray[g1].moveList == NULL)
		return COM_OK;

	/*
	 * ECO
	 */
	flag	= 0;
	i	= garray[g1].numHalfMoves;

	while (i > 0 && !flag) {
		l = 0;
		r = (ECO_entries - 1);

		while (r >= l && !flag) {
			x = ((l + r) / 2);

			if (fencmp(garray[g1].moveList[i].FENpos,
			    ECO_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!fencmp(garray[g1].moveList[i].FENpos,
			    ECO_book[x]->FENpos)) {
				pprintf(p, "  ECO[%3d]: %s\n", i,
				    ECO_book[x]->ECO);
				flag = 1;
			}
		}

		i--;
	} /* while */

	/*
	 * NIC
	 */
	flag	= 0;
	i	= garray[g1].numHalfMoves;

	while (i > 0 && !flag) {
		l = 0;
		r = (NIC_entries - 1);

		while (r >= l && !flag) {
			x = ((l + r) / 2);

			if (fencmp(garray[g1].moveList[i].FENpos,
			    NIC_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!fencmp(garray[g1].moveList[i].FENpos,
			    NIC_book[x]->FENpos)) {
				pprintf(p, "  NIC[%3d]: %s\n", i,
				    NIC_book[x]->NIC);
				flag = 1;
			}
		}

		i--;
	} /* while */

	/*
	 * LONG
	 */
	flag	= 0;
	i	= garray[g1].numHalfMoves;

	while (i > 0 && !flag) {
		l = 0;
		r = (LONG_entries - 1);

		while (r >= l && !flag) {
			x = ((l + r) / 2);

			if (fencmp(garray[g1].moveList[i].FENpos,
			    LONG_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!fencmp(garray[g1].moveList[i].FENpos,
				    LONG_book[x]->FENpos)) {
				pprintf(p, " LONG[%3d]: %s\n", i,
				    LONG_book[x]->LONG);
				flag = 1;
			}
		}

		i--;
	} /* while */
#else
	pprintf(p, "ECO not available...  out of service!.\n");
#endif
	return COM_OK;
}
