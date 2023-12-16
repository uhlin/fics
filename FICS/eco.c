#include "stdinclude.h"

#include "board.h"
#include "command.h"
#include "common.h"
#include "config.h"
#include "eco.h"
#include "gamedb.h"
#include "gameproc.h"
#include "obsproc.h"
#include "playerdb.h"
#include "utils.h"

PUBLIC char *book_dir = DEFAULT_BOOK;

ECO_entry	*ECO_book[1096];
NIC_entry	*NIC_book[1096];
LONG_entry	*LONG_book[4096];

int ECO_entries, NIC_entries, LONG_entries;

char *boardToFEN(int g)
{
  int i,j;
  static char FENstring[80];
  int FENcount = 0;
  int space = 0;

  for(i=7; i>=0; i--) {
    for(j=0; j<8; j++) {
      switch(garray[g].game_state.board[j][i]) {
      case W_PAWN:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='P';
        break;
      case W_ROOK:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='R';
        break;
      case W_KNIGHT:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='N';
        break;
      case W_BISHOP:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='B';
        break;
      case W_QUEEN:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='Q';
        break;
      case W_KING:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='K';
        break;
      case B_PAWN:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='p';
        break;
      case B_ROOK:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='r';
        break;
      case B_KNIGHT:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='n';
        break;
      case B_BISHOP:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='b';
        break;
      case B_QUEEN:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='q';
        break;
      case B_KING:
        if (space>0) {
          FENstring[FENcount++]=space+'0';
          space=0;
        }
        FENstring[FENcount++]='k';
        break;
      default:
        space++;
        break;
      }
    }
    if (space>0) {
      FENstring[FENcount++]=space+'0';
      space=0;
    }
    FENstring[FENcount++]='/';
  }
  FENstring[--FENcount]=' ';
  FENstring[++FENcount]=(garray[g].game_state.onMove==WHITE) ? 'w' : 'b';
  FENstring[++FENcount]='\0';
  return FENstring;
}

void
ECO_init()
{
	FILE	*fp;
	char	 ECO[4];
	char	 FENpos[73];
	char	 filename[1024];
	char	 onMove[2];
	char	 tmp[1024];
	char	*ptmp = tmp;
	int	 i = 0;

	sprintf(filename, "%s/eco999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Could not open ECO file (%s)\n", filename);
		exit(1);
	}

	while (!feof(fp)) {
		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);

		if (feof(fp))
			continue;

		/* XXX */
		sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
		strcat(FENpos, " ");
		strcat(FENpos, onMove);

		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);
		if (feof(fp))
			continue;
		sscanf(ptmp, "%[0-z]", ECO);

		if ((ECO_book[i] = malloc(sizeof(ECO_entry))) == NULL) {
			fprintf(stderr, "Cound not alloc mem for ECO "
			    "entry %d.\n", i);
			exit(1);
		}

		strcpy(ECO_book[i]->ECO, ECO);
		strcpy(ECO_book[i]->FENpos, FENpos);

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

void
NIC_init()
{
	FILE	*fp;
	char	 FENpos[73];
	char	 NIC[6];
	char	 filename[1024];
	char	 onMove[2];
	char	 tmp[1024];
	char	*ptmp = tmp;
	int	 i = 0;

	sprintf(filename, "%s/nic999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Could not open NIC file\n");
		exit(1);
	}

	while (!feof(fp)) {
		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);

		if (feof(fp))
			continue;

		sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
		strcat(FENpos, " ");
		strcat(FENpos, onMove);

		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);
		if (feof(fp))
			continue;
		sscanf(ptmp, "%[.-z]", NIC);

		if ((NIC_book[i] = malloc(sizeof(NIC_entry))) == NULL) {
			fprintf(stderr, "Cound not alloc mem for NIC "
			    "entry %d.\n", i);
			exit(1);
		}

		strcpy(NIC_book[i]->NIC, NIC);
		strcpy(NIC_book[i]->FENpos, FENpos);

		++i;
	}

	fclose(fp);
	NIC_book[i] = NULL;

	fprintf(stderr, "%d entries in NIC book\n", i);
	NIC_entries = i;
}

void
LONG_init()
{
	FILE	*fp;
	char	 FENpos[73];
	char	 LONG[256];
	char	 filename[1024];
	char	 onMove[2];
	char	 tmp[1024];
	char	*ptmp = tmp;
	int	 i = 0;

	sprintf(filename, "%s/long999.idx", book_dir);

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Could not open LONG file\n");
		exit(1);
	}

	while (!feof(fp)) {
		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);

		if (feof(fp))
			continue;

		/* XXX */
		sscanf(ptmp, "%[\x21-z] %s", FENpos, onMove);
		strcat(FENpos, " ");
		strcat(FENpos, onMove);

		strcpy(ptmp, "");
		fgets(ptmp, 1024, fp);
		if (feof(fp))
			continue;
		sscanf(ptmp, "%[^*\n]", LONG);

		LONG_book[i] = malloc(sizeof(LONG_entry));
		if (LONG_book[i] == NULL) {
			fprintf(stderr, "Cound not alloc mem for "
			    "LONG entry %d.\n", i);
			exit(1);
		}

		strcpy(LONG_book[i]->LONG, LONG);
		strcpy(LONG_book[i]->FENpos, FENpos);

		++i;
	}

	fclose(fp);
	LONG_book[i] = NULL;

	fprintf(stderr, "%d entries in LONG book\n", i);
	LONG_entries = i;
}

void
BookInit()
{
	ECO_init();
	NIC_init();
	LONG_init();
}

char *getECO(int g)
{
  static char ECO[4];

#ifndef IGNORE_ECO

  int i, flag, l = 0, r = ECO_entries - 1, x;


  if ((parray[garray[g].white].private) || (parray[garray[g].black].private)) {
    strcpy(ECO, "---");
    return ECO;
  } else {
    if (garray[g].type == TYPE_WILD) {
      strcpy(ECO, "---");
      return ECO;
    } else if (garray[g].moveList == NULL) {
      strcpy(ECO, "***");
      return ECO;
    } else {
      strcpy(ECO, "A00");
    }
  }

  for (flag=0,i=garray[g].numHalfMoves; (i>0 && !flag); i--) {
    l = 0;
    r = ECO_entries - 1;
    while ((r >= l) && !flag) {
      x = (l+r)/2;
      if ((strcmp(garray[g].moveList[i].FENpos, ECO_book[x]->FENpos)) < 0)
	r = x - 1;
      else
	l = x + 1;
      if (!strcmp(garray[g].moveList[i].FENpos, ECO_book[x]->FENpos)) {
        strcpy(ECO, ECO_book[x]->ECO);
        flag=1;
      }
    }
  }
#else

  strcpy(ECO, "---");

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
		if ((g1 = GameNumFromParam (p, &p1, &param[0])) < 0)
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

			if (strcmp(garray[g1].moveList[i].FENpos,
			    ECO_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!strcmp(garray[g1].moveList[i].FENpos,
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

			if (strcmp(garray[g1].moveList[i].FENpos,
			    NIC_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!strcmp(garray[g1].moveList[i].FENpos,
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

			if (strcmp(garray[g1].moveList[i].FENpos,
			    LONG_book[x]->FENpos) < 0)
				r = (x - 1);
			else
				l = (x + 1);

			if (!strcmp(garray[g1].moveList[i].FENpos,
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
