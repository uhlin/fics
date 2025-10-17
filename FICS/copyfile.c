// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "copyfile.h"

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

	close(fd[0]);
	close(fd[1]);

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
