#include "stdinclude.h"
#include "common.h"

#include "command.h"
#include "ficsmain.h"
#include "network.h"
#include "playerdb.h"
#include "shutdown.h"
#include "utils.h"

PRIVATE char	 downer[1024];
PRIVATE char	 reason[1024];

PRIVATE int	 lastTimeLeft;
PRIVATE int	 shutdownStartTime;
PRIVATE int	 shutdownTime = 0;

PUBLIC void
output_shut_mess()
{
	time_t shuttime = time(NULL);

	fprintf(stderr, "FICS: Shutting down at %s\n", strltime(&shuttime));
}

PUBLIC void
ShutDown(void)
{
	time_t shuttime = time(NULL);

	for (int p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status != PLAYER_PROMPT)
			continue;

		pprintf(p1, "\n\n    **** Server shutdown ordered by %s. "
		    "****\n", downer);

		if (reason[0] != '\0') {
			pprintf(p1, "\n    **** We are going down because: "
			    "%s. ****\n", reason);
		}
	}

	TerminateCleanup();
	fprintf(stderr, "FICS: Shut down ordered at %s by %s.\n",
	    strltime(&shuttime), downer);
	output_shut_mess();
	net_close();
	exit(0);
}

PUBLIC void
ShutHeartBeat(void)
{
	int	crossing = 0;
	int	p1;
	time_t	t = time(NULL);
	int	timeLeft;

	if (!shutdownTime)
		return;
	if (!lastTimeLeft)
		lastTimeLeft = shutdownTime;

	timeLeft = shutdownTime - (t - shutdownStartTime);

	if (lastTimeLeft > 3600 && timeLeft <= 3600)
		crossing = 1;
	if (lastTimeLeft > 2400 && timeLeft <= 2400)
		crossing = 1;
	if (lastTimeLeft > 1200 && timeLeft <= 1200)
		crossing = 1;
	if (lastTimeLeft > 600 && timeLeft <= 600)
		crossing = 1;
	if (lastTimeLeft > 300 && timeLeft <= 300)
		crossing = 1;
	if (lastTimeLeft > 120 && timeLeft <= 120)
		crossing = 1;
	if (lastTimeLeft > 60 && timeLeft <= 60)
		crossing = 1;
	if (lastTimeLeft > 10 && timeLeft <= 10)
		crossing = 1;

	if (crossing) {
		fprintf(stderr, "FICS:   **** Server going down in %d minutes "
		    "and %d seconds. ****\n\n",
		    (timeLeft / 60),
		    timeLeft - ((timeLeft / 60) * 60));

		if (reason[0] != '\0') {
			fprintf(stderr,"FICS: We are going down because: %s.\n",
			    reason);
		}

		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;

			pprintf(p1, "\n\n    **** Server going down in %d "
			    "minutes and %d seconds. ****\n",
			    (timeLeft / 60),
			    timeLeft - ((timeLeft / 60) * 60));

			if (reason[0] != '\0') {
				pprintf_prompt(p1, "\n    **** We are going "
				    "down because: %s. ****\n", reason);
			} else
				pprintf_prompt(p1, "\n");
		}
	}

	lastTimeLeft = timeLeft;

	if (timeLeft <= 0)
		ShutDown();
}

/*
 * Tells a user about a shutdown. Returns 1 if there is to be one, and
 * 0 otherwise. (For 'whenshut' command.)
 */
PUBLIC int
check_and_print_shutdown(int p)
{
	int	timeLeft;
	time_t	t = time(NULL);

	if (!shutdownTime)
		return 0;     // no shutdown

	timeLeft = shutdownTime - (t - shutdownStartTime);

	pprintf(p, "\n    **** Server going down in %d minutes and %d seconds. "
	    "****\n",
	    (timeLeft / 60),
	    timeLeft - ((timeLeft / 60) * 60));

	if (reason[0] != '\0') {
		pprintf(p, "\n    **** We are going down because: %s. ****\n",
		    reason);
	}
	return 1;
}


/*
 * Usage: shutdown [now,cancel,time]
 *
 * This command shuts down the server. If the parameter is omitted or
 * is 'now' then the server is immediately halted cleanly. If a time
 * is given then a countdown commences and the server is halted when
 * time is up. If 'cancel' is given then the countdown is stopped.
 */
PUBLIC int
com_shutdown(int p, param_list param)
{
	char	*ptr;
	int	 p1, secs;

	ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
	strcpy(downer, parray[p].name);
	shutdownStartTime = time(0);

	if (shutdownTime) {   // Cancel any pending shutdowns
		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			pprintf(p1, "\n\n    **** Server shutdown canceled by "
			    "%s. ****\n", downer);
		}

		shutdownTime = 0;

		if (param[0].type == TYPE_NULL)
			return COM_OK;
	}

	/*
	 * Work out how soon to shut down
	 */
	if (param[0].type == TYPE_NULL) {
		shutdownTime = 300;
	} else {
		if (!strcmp(param[0].val.word, "now")) {
			shutdownTime = 0;
		} else if (!strcmp(param[0].val.word, "die")) {
			fprintf(stderr,"%s salutes FICS and presses the "
			    "self-destruct button.\n", parray[p].name);
			output_shut_mess();
			abort();
		} else if (!strcmp(param[0].val.word, "cancel")) {
			return COM_OK;
		} else {
			ptr = param[0].val.word;
			shutdownTime = secs = 0;
			p1 = 2;

			while (*ptr) {
				switch (*ptr) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					secs = (secs * 10 + *ptr - '0');
					break;
				case ':':
					if (p1--) {
						shutdownTime = (shutdownTime *
						    60 + secs);
						secs = 0;
						break;
					}
				default:
					shutdownTime = 0;
					pprintf(p, "I don't know what you mean "
					    "by %s\n", param[0].val.word);
					return COM_OK;
				}

				ptr++;
			}

			shutdownTime = (shutdownTime * 60 + secs);
		}
	}

	if (shutdownTime <= 0)
		ShutDown();
	if (param[1].type == TYPE_STRING)
		strcpy(reason, param[1].val.string);
	else
		reason[0] = '\0';	// No reason - perhaps admin is in a
					// bad mood? :)

	for (p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status != PLAYER_PROMPT)
			continue;

		pprintf(p1, "\n\n    **** Server shutdown ordered by %s. "
		    "****\n", downer);

		if (reason[0] != '\0') {
			pprintf(p1, "    **** We are going down because: "
			    "%s. ****\n", reason);
		}

		pprintf(p1, "    **** Server going down in %d minutes and %d "
		    "seconds. ****\n",
		    (shutdownTime / 60),
		    (shutdownTime % 60));
		if (p != p1)  // fix double prompt - DAV
			pprintf_prompt(p1, "\n");
		else
			pprintf(p1, "\n");
	}

	lastTimeLeft = 0;
	return COM_OK;
}

PUBLIC int
server_shutdown(int secs, char *why)
{
	if (shutdownTime && shutdownTime <= secs) {
		/*
		 * Server is already shutting down
		 */
		return 0;
	}

	strcpy(downer, "Automatic");
	shutdownTime = secs;
	shutdownStartTime = time(NULL);

	for (int p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status != PLAYER_PROMPT)
			continue;
		pprintf(p1, "\n\n    **** Automatic Server shutdown. ****\n");
		pprintf(p1, "%s\n", why);
		pprintf_prompt(p1, "    **** Server going down in %d minutes "
		    "and %d seconds. ****\n\n",
		    (shutdownTime / 60),
		    shutdownTime - ((shutdownTime / 60) * 60));
	}

	fprintf(stderr, "FICS:    **** Automatic Server shutdown. ****\n");
	fprintf(stderr, "FICS: %s\n", why);
	return 0;
}

PUBLIC int
com_whenshut(int p, param_list param)
{
	if (check_and_print_shutdown(p) == 0)
		pprintf(p, "No shutdown currently in progress\n");
	return COM_OK;
}
