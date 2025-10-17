// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "copyfile.h"

#define PRINT_CHECKSUMS 0
#define SELF_TEST 0

bool
fics_copyfile(const char *p1, const char *p2)
{
	char	tmp[2048] = { '\0' };
	int	fd[2];
	ssize_t ret[2];
	ssize_t total_read = 0;
	ssize_t total_written = 0;

	if (p1 == NULL || p2 == NULL ||
	    strcmp(p1, "") == 0 ||
	    strcmp(p2, "") == 0)
		return false;
	if (!is_regular_file(p1)) {
		warnx("%s: not a regular file", __func__);
		return false;
	}
	if ((fd[0] = open(p1, O_RDONLY)) < 0) {
		warn("%s: open(%s, ...)", __func__, p1);
		return false;
	}

	fd[1] = open(p2, (O_RDWR|O_APPEND|O_CREAT|O_TRUNC),
	    (S_IRUSR|S_IWUSR|S_IRGRP));
	if (fd[1] < 0) {
		warn("%s: open(%s, ...)", __func__, p2);
		close(fd[0]);
		return false;
	}

	while ((ret[0] = read(fd[0], tmp, sizeof tmp)) != -1 && ret[0] != 0) {
		total_read += ret[0];

		if ((ret[1] = write(fd[1], tmp, ret[0])) != ret[0]) {
			warnx("%s: written mismatch read", __func__);
			break;
		}

		total_written += ret[1];
	}

	if (close(fd[0]) != 0)
		perror("close");
	if (close(fd[1]) != 0)
		perror("close");

	if (total_read != total_written) {
		warnx("%s: total written mismatch total read", __func__);
		return false;
	}

	return true;
}

bool
is_regular_file(const char *path)
{
	struct stat sb = { 0 };

	if (path == NULL || strcmp(path, "") == 0)
		return false;
	return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}

#if SELF_TEST
int
main(int argc, char *argv[])
{
	if (argc != 3)
		errx(1, "bogus number of args");
	return (fics_copyfile(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE);
}
#endif
