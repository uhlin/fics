// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <string>
#include <vector>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "addgroup.h"

#define SELF_TEST 0

struct group_info {
	std::string	name;
	std::string	password;
	int		gid;
	std::string	members;

	group_info()
	{
		this->name.assign("");
		this->password.assign("");
		this->gid = 0;
		this->members.assign("");
	}

	group_info(const char *p_name, const char *p_password, const int p_gid,
		   const char *p_members)
	{
		this->name.assign(p_name);
		this->password.assign(p_password);
		this->gid = p_gid;
		this->members.assign(p_members);
	}
};

static std::vector<group_info> groups;

static const int	FIRST_SYSTEM_GID = 100;
static const int	LAST_SYSTEM_GID = 999;

static bool
is_free_gid(const int gid)
{
	for (auto it = groups.begin(); it != groups.end(); ++it) {
		if ((*it).gid == gid)
			return false;
	}
	return true;
}

static int
get_free_gid(void)
{
	if (groups.empty())
		return -1;
	for (int gid = FIRST_SYSTEM_GID; gid <= LAST_SYSTEM_GID; gid++) {
		if (is_free_gid(gid))
			return gid;
	}
	return -1;
}

int
fics_addgroup(const char *name)
{
	int	fd;
	int	gid;

	if (name == nullptr || strcmp(name, "") == 0)
		return -1;
	else if (group_exists(name))
		return 0;
	else if ((gid = get_free_gid()) == -1)
		return -1;
	fd = open("/etc/group", (O_RDWR|O_APPEND));
	if (fd < 0)
		return -1;
	struct group_info group(name, "*", gid, "");
	dprintf(fd, "%s:%s:%d:%s\n", group.name.c_str(), group.password.c_str(),
	    group.gid, group.members.c_str());
	close(fd);
	groups.push_back(group);
	return 0;
}

bool
get_group_id(const char *name, int *gid)
{
	if (name == nullptr || strcmp(name, "") == 0 || gid == nullptr ||
	    groups.empty()) {
		if (gid)
			*gid = 0;
		return false;
	}

	for (auto it = groups.begin(); it != groups.end(); ++it) {
		if (strcmp((*it).name.c_str(), name) == 0) {
			*gid = (*it).gid;
			return true;
		}
	}

	*gid = 0;
	return false;
}

bool
get_next_line_from_file(FILE *fp, char **line)
{
	const int LINE_MAX_LEN = 2048;

	if (fp == nullptr || line == nullptr)
		errx(1, "%s: invalid argument", __func__);

	if (*line) {
		delete[] *line;
		*line = nullptr;
	}

	*line = new char[LINE_MAX_LEN];

	return (fgets(*line, LINE_MAX_LEN, fp) ? true : false);
}

bool
group_exists(const char *name)
{
	if (name == nullptr || strcmp(name, "") == 0 || groups.empty())
		return false;
	for (auto it = groups.begin(); it != groups.end(); ++it) {
		if (strcmp((*it).name.c_str(), name) == 0)
			return true;
	}
	return false;
}

bool
is_valid_group_name(const char *name)
{
	const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789_-";
	const size_t name_min = 2;
	const size_t name_max = 30;

	if (name == nullptr || strcmp(name, "") == 0)
		return false;
	else if (strlen(name) < name_min || strlen(name) > name_max)
		return false;
	for (const char *cp = name; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == nullptr)
			return false;
	}
	return true;
}

int
read_the_group_permissions_file(const char *path)
{
	FILE		*fp = nullptr;
	bool		 read_ok = false;
	char		*line = nullptr;
	const char	*token[4];
	const char	 delim[] = ":";
	int		 gid = 0;

	if (!groups.empty())
		return 0;
	if ((fp = fopen(path, "r")) == nullptr)
		return -1;
	while (get_next_line_from_file(fp, &line)) {
		token[0] = strsep(&line, delim);
		token[1] = strsep(&line, delim);
		token[2] = strsep(&line, delim);
		token[3] = strsep(&line, delim);

		if (token[0] == nullptr ||
		    token[1] == nullptr ||
		    token[2] == nullptr ||
		    token[3] == nullptr) {
			warnx("%s: too few tokens", __func__);
			continue;
		} else if (sscanf(token[2], "%d", &gid) != 1) {
			warnx("%s: sscanf() error", __func__);
			continue;
		}

		struct group_info group(token[0], token[1], gid, token[3]);
		groups.push_back(group);
	}

	if (feof(fp))
		read_ok = true;

	fclose(fp);
	delete[] line;

	return (read_ok ? 0 : -1);
}

#if SELF_TEST
int
main(void)
{
	if (read_the_group_permissions_file("/etc/group") == -1)
		errx(1, "failed to read the group permissions file");
	if (fics_addgroup("chess") == -1)
		errx(1, "failed to add a group");
	puts("ok");
	return 0;
}
#endif
