/*
 * Formula program for FICS. Written by Dave Herscovici
 *                           <dhersco@math.nps.navy.mil>
 * Edited by DAV to include wild, non-std and untimed lightning flags.
 *
 * Operators allowed, in order of precedence:
 *     ! (not), - (unary),
 *     *, /,
 *     +, -,
 *     <, <=, =<, >, >=, =>, (<= and =< are equivalent, as are >= and =>)
 *     =, ==, !=, <>, (two ways of saying both equals and not equals)
 *     &, &&, (both "and")
 *     |, ||, (both "or").
 *
 * Parentheses () are also allowed, as is a pound sign '#' for comments.
 * The program will divide by a fudge factor of .001 instead of 0.
 *
 * Variables allowed:
 *     assessdraw, assessloss, assesswin, blitz,
 *     f1, f2, f3, f4, f5, f6, f7, f8, f9,
 *     inc, lightning,
 *     maxtime(n) = maximum time n moves will take (in seconds),
 *     mymaxtime(n) = same, but just count my time,
 *     myrating, nonstandard, private, rated, rating,
 *     ratingdiff = rating - myrating,
 *     registered, standard, time, timeseal, untimed, wild.
 *
 * The code for translating blitz and standard variables may have to
 * be redone. f1 through f9 are user-defined formula variables. They
 * can be used to avoid having to retype your entire formula when you
 * want to change one part of it. Or to compensate for the lack of a
 * 'mood' variable.
 *
 * For example:
 *     set f1 rated & time=5 & inc=0    # Rated 5 minute games
 *     set f2 rating - myrating
 *     set f3 # This line is a random comment
 *     set f4 f2>400 # I want a REAL fight
 *
 * Then you can type:
 *     set formula f1    # Rated 5 min games only
 *     set formula etime >= 10 & f2 > -100 # Long games, decent competition.
 *     set formula f1 & !f4
 * Or 'set formula f2 >= 0 | blitz' depending on your mood.
 *
 * Further modifications could account for starting positions, time
 * odds games, provisional or established opponents, etc. Maybe f0
 * should be reserved for your formula upon logging in, i.e. if f0 is
 * set, your formula is automatically set to f0 when you log in.
 */

#include "stdinclude.h"
#include "common.h"

#include <ctype.h>

#include "command.h"
#include "config.h"
#include "formula.h"
#include "gamedb.h"
#include "lists.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "utils.h"

PRIVATE char *
GetPlayersFormula(player *pWho, int clause)
{
	if (clause == MAX_FORMULA)
		return pWho->formula;
	return pWho->formulaLines[clause];
}

PRIVATE int
MoveIndexPastString(char *string, int *i, char *text)
{
	int n = strlen(text);

	if (strncasecmp(text, string + *i, n))
		return 0;
	*i += n;
	return n;
}

PUBLIC int
GetRating(player *p, int gametype)
{
	if (gametype == TYPE_BLITZ)
		return (p->b_stats.rating);
	else if (gametype == TYPE_STAND)
		return (p->s_stats.rating);
	else if (gametype == TYPE_WILD)
		return (p->w_stats.rating);
	else if (gametype == TYPE_LIGHT)
		return (p->l_stats.rating);
	else if (gametype == TYPE_BUGHOUSE)
		return (p->bug_stats.rating);
	else
		return 0;
}

PRIVATE int
GetNumberInsideParens(game *g, int clause, int *i, int *token, int eval)
{
	char	*string = GetPlayersFormula(&parray[g->black], clause);
	int	 ret;

	while (string[*i] != '\0' && isspace(string[*i]))
		(*i)++;

	if (!MoveIndexPastString(string, i, "("))
		return ERR_BADCHAR;

	ret = CheckFormula(g, clause, i, OPTYPE_PAREN, token, eval);

	if (ret != ERR_NONE)
		return ret;

	if (MoveIndexPastString(string, i, ")"))
		return ERR_NONE;
	else
		return ERR_PAREN;
}

PRIVATE int
Maxtime(game *g, int numMoves, int numPlayers)
{
	int max;

	if (g->bInitTime == g->wInitTime && g->bIncrement == g->wIncrement) {
		max = numPlayers * (60 * g->wInitTime + numMoves *
		    g->wIncrement);

		if (g->type != TYPE_UNTIMED && g->wInitTime == 0)
			max += (10 * numPlayers);
	} else if (numPlayers == 2) {
		max = 60 * (g->wInitTime + g->bInitTime) + numMoves *
		    (g->wIncrement + g->bIncrement);

		if (g->type != TYPE_UNTIMED && g->wInitTime == 0)
			max += 10;
		if (g->type != TYPE_UNTIMED && g->bInitTime == 0)
			max += 10;
	} else {
		max = (60 * g->bInitTime + numMoves * g->bIncrement);

		if (g->type != TYPE_UNTIMED && g->bInitTime == 0)
			max += 10;
	}
	return max;
}

/*
 * The black player in game 'g' has been challenged. S/he has a list
 * of formulas and we're checking the one given by the index
 * clause. 'MAX_FORMULA' denotes the actual formula. If clause is
 * smaller, we're looking at one of the user-defined formulas. We're
 * at position 'i' in the formula. If we find a legitimate variable
 * there and 'eval' is 1 VarToToken() puts its value in 'tok'. This
 * function returns 1 or 0 depending on whether a legitimate variable
 * was found.
 */
PRIVATE int
VarToToken(game *g, int clause, char *string, int *i, int *tok, int eval)
{
	double	 dummy_sterr;
	int	 index = 0, c;
	player	*me = &parray[g->black];
	player	*you = &parray[g->white];

	/*
	 * We list possible variables with the longest names first.
	 */
	if (MoveIndexPastString(string, i, "registered")) {
		*tok = you->registered;
	} else if (MoveIndexPastString(string, i, "ratingdiff")) {
		*tok = (GetRating(you, g->type) - GetRating(me, g->type));
	} else if (MoveIndexPastString(string, i, "assessloss")) {
		if (g->rated) {
			rating_sterr_delta(g->black, g->white, g->type,
			    time(NULL), RESULT_LOSS, tok, &dummy_sterr);
		} else {
			*tok = 0;
		}
	} else if (MoveIndexPastString(string, i, "assessdraw")) {
		if (g->rated) {
			rating_sterr_delta(g->black, g->white, g->type,
			    time(NULL), RESULT_DRAW, tok, &dummy_sterr);
		} else {
			*tok = 0;
		}
	} else if (MoveIndexPastString(string, i, "assesswin")) {
		if (g->rated) {
			rating_sterr_delta(g->black, g->white, g->type,
			    time(NULL), RESULT_WIN, tok, &dummy_sterr);
		} else {
			*tok = 0;
		}
	} else if (MoveIndexPastString(string, i, "mymaxtime")) {
		if (GetNumberInsideParens(g, clause, i, tok, eval) != ERR_NONE)
			return 0;
		*tok = Maxtime(g, *tok, 1);
	}
#ifdef TIMESEAL
	else if (MoveIndexPastString(string, i, "timeseal"))
		*tok = con[you->socket].timeseal;
#endif
	else if (MoveIndexPastString(string, i, "myrating"))
		*tok = GetRating(me, g->type);
	else if (MoveIndexPastString(string, i, "computer"))
		*tok = in_list(-1, L_COMPUTER, you->name);
	else if (MoveIndexPastString(string, i, "standard"))
		*tok = (g->type == TYPE_STAND);
	else if (MoveIndexPastString(string, i, "private"))
		*tok = you->private;
	else if (MoveIndexPastString(string, i, "maxtime")) {
		if (GetNumberInsideParens(g, clause, i, tok, eval) != ERR_NONE)
			return 0;
		*tok = Maxtime(g, *tok, 2);
	} else if (MoveIndexPastString(string, i, "abuser"))
		*tok = in_list(-1, L_ABUSER, you->name);
	else if (MoveIndexPastString(string, i, "rating"))
		*tok = GetRating(you, g->type);
	else if (MoveIndexPastString(string, i, "rated"))
		*tok = g->rated;
	else if (MoveIndexPastString(string, i, "blitz"))
		*tok = (g->type == TYPE_BLITZ);
	else if (MoveIndexPastString(string, i, "wild"))
		*tok = (g->type == TYPE_WILD);
	else if (MoveIndexPastString(string, i, "lightning"))
		*tok = (g->type == TYPE_LIGHT);
	else if (MoveIndexPastString(string, i, "nonstandard"))
		*tok = (g->type == TYPE_NONSTANDARD);
	else if (MoveIndexPastString(string, i, "untimed"))
		*tok = (g->type == TYPE_UNTIMED);
	else if (MoveIndexPastString(string, i, "time"))
		*tok = g->wInitTime;
	else if (MoveIndexPastString(string, i, "inc"))
		*tok = g->wIncrement;
	else if (tolower(string[*i]) == 'f' &&
	    isdigit(string[*i + 1]) &&
	    (c = (string[*i + 1] - '1')) >= 0 &&
	    clause > c) {
		*i += 2;
		return (!CheckFormula(g, c, &index, OPTYPE_NONE, tok, eval));
	} else
		return 0;
	return 1;
}

/*
 * ScanForOp() checks for an operator at position 'i' in 'string'.
 */
PRIVATE int
ScanForOp(char *string, int *i)
{
	while (string[*i] != '\0' && isspace(string[*i]))
		(*i)++;

	if (string[*i] == '\0')
		return OP_NONE;
	if (string[*i] == '#')
		return OP_NONE;
	if (string[*i] == ')')
		return OP_RTPAREN;

	/*
	 * Two char operators and longer first.
	 */
	if (MoveIndexPastString(string, i, "and"))
		return OP_AND;
	if (MoveIndexPastString(string, i, "or"))
		return OP_OR;
	if (MoveIndexPastString(string, i, "||"))
		return OP_OR;
	if (MoveIndexPastString(string, i, "&&"))
		return OP_AND;
	if (MoveIndexPastString(string, i, "=="))
		return OP_EQ;
	if (MoveIndexPastString(string, i, "!="))
		return OP_NEQ;
	if (MoveIndexPastString(string, i, "<>"))
		return OP_NEQ;
	if (MoveIndexPastString(string, i, ">="))
		return OP_GE;
	if (MoveIndexPastString(string, i, "=>"))
		return OP_GE;
	if (MoveIndexPastString(string, i, "<="))
		return OP_LE;
	if (MoveIndexPastString(string, i, "=<"))
		return OP_LE;

	/*
	 * One char operators now.
	 */
	if (MoveIndexPastString(string, i, "|"))
		return OP_OR;
	if (MoveIndexPastString(string, i, "&"))
		return OP_AND;
	if (MoveIndexPastString(string, i, ">"))
		return OP_GT;
	if (MoveIndexPastString(string, i, "<"))
		return OP_LT;
	if (MoveIndexPastString(string, i, "="))
		return OP_EQ;
	if (MoveIndexPastString(string, i, "+"))
		return OP_PLUS;
	if (MoveIndexPastString(string, i, "-"))
		return OP_MINUS;
	if (MoveIndexPastString(string, i, "*"))
		return OP_MULT;
	if (MoveIndexPastString(string, i, "/"))
		return OP_DIV;

	return OP_BAD;
}

/*
 * OpType() returns the precedence of the operator 'op'.
 */
PRIVATE int
OpType(int op)
{
	switch (op) {
	case OP_BAD:
		return OPTYPE_BAD;
	case OP_NONE:
		return OPTYPE_NONE;
	case OP_RTPAREN:
		return OPTYPE_RPAREN;
	case OP_OR:
		return OPTYPE_OR;
	case OP_AND:
		return OPTYPE_AND;
	case OP_EQ:
	case OP_NEQ:
		return OPTYPE_COMPEQ;
	case OP_GT:
	case OP_GE:
	case OP_LT:
	case OP_LE:
		return OPTYPE_COMPGL;
	case OP_PLUS:
	case OP_MINUS:
		return OPTYPE_ADD;
	case OP_MULT:
	case OP_DIV:
		return OPTYPE_MULT;
	case OP_NEGATE:
	case OP_NOT:
		return OPTYPE_UNARY;
	default:
		return OPTYPE_BAD;
	}
}

/*
 * In EvalBinaryOp() 'left' is the left operand, and 'op' is the
 * current operator. 'g' and 'clause' specify which formula string to
 * look at, and 'i' tells us where we are in the string. We look for a
 * right operand from position 'i' in the string. And place the
 * expression in 'left'. This function returns 0 if no error.
 * Otherwise an error code is returned.
 */
PRIVATE int
EvalBinaryOp(int *left, int op, game *g, int clause, int *i)
{
	int right, ret;

	if (op == OP_OR && *left != 0) { /* Nothing to decide. */
		*left = 1;
		return CheckFormula(g, clause, i, OpType(op), &right, 0);
	} else if (op == OP_AND && *left == 0) { /* Nothing to decide. */
		return CheckFormula(g, clause, i, OpType(op), &right, 0);
	} else {
		ret = CheckFormula(g, clause, i, OpType(op), &right, 1);

		if (ret != ERR_NONE)
			return ret;
	}

	switch (op) {
	default:
	case OP_BAD:
		return ERR_BADOP;
	case OP_NONE:
	case OP_RTPAREN:
		return ERR_NONE;
	case OP_OR:
		*left = (*left || right);
		return ERR_NONE;
	case OP_AND:
		*left = (*left && right);
		return ERR_NONE;
	case OP_EQ:
		*left = (*left == right);
		return ERR_NONE;
	case OP_NEQ:
		*left = (*left != right);
		return ERR_NONE;
	case OP_GT:
		*left = (*left > right);
		return ERR_NONE;
	case OP_GE:
		*left = (*left >= right);
		return ERR_NONE;
	case OP_LT:
		*left = (*left < right);
		return ERR_NONE;
	case OP_LE:
		*left = (*left <= right);
		return ERR_NONE;
	case OP_PLUS:
		*left += right;
		return ERR_NONE;
	case OP_MINUS:
		*left -= right;
		return ERR_NONE;
	case OP_MULT:
		*left *= right;
		return ERR_NONE;
	case OP_DIV:
		if (right != 0) {
			*left /= right;
			return ERR_NONE;
		} else {
			pprintf(g->black, "Dividing by %lf instead or zero in "
			    "formula.\n", FUDGE_FACTOR);
			*left /= FUDGE_FACTOR;
			return ERR_NONE;
		}
	} /* switch */
}

/*
 * If 'eval' is 1, ScanForNumber() evaluates the number at position
 * 'i' in the formula given by 'g' and 'clause', and places this value
 * in 'token'. 'op_type' is the precedence of the operator preceding
 * the i'th position. If we come to an operator of higher precedence
 * we must keep going before we can leave this function. If 'eval' is
 * 0, just move past the number we would evaluate. This function
 * returns 0 if no error. Otherwise an error code is returned.
 */
PRIVATE int
ScanForNumber(game *g, int clause, int *i, int op_type, int *token, int eval)
{
	char	*string = GetPlayersFormula(&parray[g->black], clause);
	char	 c;
	int	 ret;

	while (string[*i] != '\0' && isspace(string[*i]))
		(*i)++;

	switch (c = string[*i]) {
	case '\0':
	case '#':
		if (op_type != OPTYPE_NONE)
			return ERR_EOF;
		else
			*token = 1;
		return ERR_NONE;
	case ')':
		if (op_type != OPTYPE_PAREN)
			return ERR_PAREN;
		else
			*token = 1;
		return ERR_NONE;
	case '(':
		return GetNumberInsideParens(g, clause, i, token, eval);
	case '-':
	case '!':
		++(*i);

		if (c == string[*i])
			return ERR_UNARY;

		ret = ScanForNumber(g, clause, i, OPTYPE_UNARY, token, eval);

		if (ret != ERR_NONE)
			return ret;
		if (c == '-')
			*token = -(*token);
		else if (c == '!')
			*token = !(*token);
		return ERR_NONE;
	default:
		if (isdigit(string[*i])) {
			*token = 0;

			do {
				*token = (10 * (*token) + string[(*i)++] - '0');
			} while (isdigit(string[*i]));

			while (string[*i] != '\0' && isspace(string[*i]))
				(*i)++;

			if (MoveIndexPastString(string, i, "minutes"))
				*token *= 60;
			return ERR_NONE;
		} else if (isalpha(string[*i])) {
			if (!VarToToken(g, clause, string, i, token, eval))
				return ERR_BADVAR;
			return ERR_NONE;
		} else
			return ERR_NONESENSE;
	} /* switch */
}

/*
 * If 'eval' is 1, CheckFormula() looks for the next token in the
 * formula given by 'g', 'clause' and 'i'. Usually this is the right
 * side of an expression whose operator has precedence OpType(). If
 * 'eval' is 0, just go to the end of an expression.
 */
PUBLIC int
CheckFormula(game *g, int clause, int *i, int op_type, int *result, int eval)
{
	char	*string = GetPlayersFormula(&parray[g->black], clause);
	int	 token, ret, nextOp, lastPos;

	if (string == NULL) {
		*result = 1;
		return ERR_NONE;
	}

	ret = ScanForNumber(g, clause, i, op_type, &token, eval);

	if (ret != ERR_NONE)
		return ret;

	lastPos	= *i;
	nextOp	= ScanForOp(string, i);

	while (OpType(nextOp) > op_type) { /* Higher precedence. */
			if (nextOp == OP_RTPAREN)
				return ERR_PAREN;
			if (!eval) {
				ret = CheckFormula(g, clause, i, OpType(nextOp),
				    &token, 0);
			} else {
				ret = EvalBinaryOp(&token, nextOp, g, clause,
				    i);
			}

			if (ret != ERR_NONE)
				return ret;
			lastPos	= *i;
			nextOp	= ScanForOp(string, i);
	}

	if (nextOp == OP_BAD)
		return ERR_BADOP;
	*result = token;
	/*
	 * Move back unless we're at the end or at a right paren, in
	 * which case we never went forward.
	 */
	if (nextOp != OP_NONE && nextOp != OP_RTPAREN)
		*i = lastPos;
	return ERR_NONE;
}

/*
 * Which clauses are relevant for a player's formula.
 */
PRIVATE int
ChooseClauses(player *who, char *formula)
{
	int i, which, ret = 0;

	if (formula == NULL)
		return ret;

	for (i = 0; formula[i] != '\0' && formula[i] != '#'; i++) {
		if ((i > 0 && isalnum(formula[i - 1])) || formula[i] != 'f' ||
		    !isdigit(formula[i + 1]) ||
		    sscanf(&formula[i], "f%d", &which) != 1)
			continue;

		ret |= (1 << (which - 1));
	}

	/* Now scan clauses found as part of the formula. */
	for (i = MAX_FORMULA - 1; i >= 0; i--) {
		if (ret & (1 << i))
			ret |= ChooseClauses(who, who->formulaLines[i]);
	}

	return ret;
}

PRIVATE void
ExplainFormula(game *g, textlist **clauses)
{
	char		 txt[20] = { '\0' };
	int		 i, which, dummy_index, value;
	player		*challenged = &parray[g->black];
	textlist**	 Cur = clauses;

	which = ChooseClauses(challenged, challenged->formula);

	for (i = 0; i < MAX_FORMULA; i++) {
		if ((which & (1 << i)) == 0)
			continue;
		dummy_index = 0;
		CheckFormula(g, i, &dummy_index, OPTYPE_NONE, &value, 1);
		snprintf(txt, sizeof txt, "%d", value);
		SaveTextListEntry(Cur, txt, i);
		Cur = &(*Cur)->next;
	}
}

/*
 * GameMatchesFormula() converts parameters to a game structure and
 * passes a pointer to this game to CheckFormula() for evaluation. It
 * returns the evaluated formula.
 */
PUBLIC int
GameMatchesFormula(int w, int b, int wTime, int wInc, int bTime, int bInc,
    int rated, int gametype, textlist **clauseList)
{
	game	g;
	int	index = 0, ret;

	g.white		= w;
	g.black		= b;
	g.wInitTime	= wTime;
	g.bInitTime	= bTime;
	g.wIncrement	= wInc;
	g.bIncrement	= bInc;
	g.rated		= rated;
	g.type		= gametype;

	if (CheckFormula(&g, MAX_FORMULA, &index, OPTYPE_NONE, &ret, 1) !=
	    ERR_NONE)
		return 0;
	if (ret == 0)
		ExplainFormula(&g, clauseList);
	return ret;
}

/*
 * SetValidFormula() sets a clause of player 'p' and creates a game
 * structure to check whether that new formula is legitimate. If so,
 * return 1. Otherwise reset the formula and return 0.
 */
PUBLIC int
SetValidFormula(int p, int clause, char *string)
{
	char	*Old = NULL, **Cur;
	game	 g;
	int	 index = 0, ret, err = ERR_NONE;
	player	*me = &parray[p];

	if (clause == MAX_FORMULA)
		Cur = &me->formula;
	else
		Cur = &me->formulaLines[clause];

	Old = *Cur;

	if (string != NULL) {
		string = eatwhite(string);
		*Cur = (*string != '\0' ? xstrdup(string) : NULL);
	} else
		*Cur = NULL;

	if (*Cur == NULL) {
		if (Old != NULL)
			rfree(Old);
		return 1;
	}

	g.white = g.black = p;
	g.wInitTime = g.bInitTime = me->d_time;
	g.wIncrement = g.bIncrement = me->d_inc;
	g.rated = me->rated;
	g.type = TYPE_BLITZ;

	err = CheckFormula(&g, clause, &index, OPTYPE_NONE, &ret, 0);

	if (err != ERR_NONE) {
		/* Bad formula  --  reset it. */
		rfree(*Cur);
		*Cur = Old;
	} else {
		if (Old != NULL)
			rfree(Old);
	}

	return (err == ERR_NONE);
}

PUBLIC void
ShowClauses(int p, int p1, textlist *clauses)
{
	if (clauses != NULL)
		pprintf(p, "\n");
	for (textlist *Cur = clauses; Cur != NULL; Cur = Cur->next) {
		pprintf(p, "f%d=%s: %s\n", (Cur->index + 1), Cur->text,
		    parray[p1].formulaLines[Cur->index]);
	}
}
