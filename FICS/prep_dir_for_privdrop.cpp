// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <string>

#include "addgroup.h"
#include "prep_dir_for_privdrop.h"
#include "settings.h"
#include "utils.h"

namespace fs = std::filesystem;

static int
get_uid_and_gid(uid_t &uid, gid_t &gid)
{
	struct passwd	*pw = nullptr;
	int		 i = 0;

	if ((pw = getpwnam(settings_get("privdrop_user"))) == nullptr ||
	    !get_group_id(settings_get("sysgroup"), &i)) {
		uid = 0;
		gid = 0;
		return -1;
	}

	uid = pw->pw_uid;
	gid = static_cast<gid_t>(i);
	return 0;
}

static int
check_prep_done(const char *p_path)
{
	std::string path(p_path);

	path.append("/").append(".prep_done");

	if (file_exists(path.c_str()))
		return 0;
	return -1;
}

static void
prep_done(const char *p_path)
{
	int		fd;
	std::string	path(p_path);
	uid_t		uid = 0;
	gid_t		gid = 0;

	path.append("/").append(".prep_done");

	fd = open(path.c_str(), (O_RDWR|O_CREAT|O_TRUNC), (S_IRUSR|S_IWUSR|
	    S_IRGRP|S_IROTH));
	if (fd < 0) {
		warn("%s: open", __func__);
		return;
	}

	dprintf(fd, "FICS dir preparation done.\n"
	    "Do not remove this file while FICS is installed.\n");
	close(fd);

	if (get_uid_and_gid(uid, gid) == 0) {
		if (chown(path.c_str(), uid, gid) != 0)
			warn("%s: chown", __func__);
	}
}

int
drop_root_privileges(const char *path)
{
	struct passwd *pw;

	if ((pw = getpwnam(settings_get("privdrop_user"))) == nullptr) {
		warnx("%s: password database search failed", __func__);
		return -1;
	} else if (chdir(path) != 0) {
		warn("%s: chdir(%s)", __func__, path);
		return -1;
	} else if (setgid(pw->pw_gid) == -1) {
		warn("%s: setgid", __func__);
		return -1;
	} else if (setegid(pw->pw_gid) == -1) {
		warn("%s: setegid", __func__);
		return -1;
	} else if (setuid(pw->pw_uid) == -1) {
		warn("%s: setuid", __func__);
		return -1;
	} else if (seteuid(pw->pw_uid) == -1) {
		warn("%s: seteuid", __func__);
		return -1;
	}

	return 0;
}

int
prep_dir_for_privdrop(const char *path)
{
	if (path == nullptr || strcmp(path, "") == 0)
		return -1;
	else if (check_prep_done(path) == 0)
		return 0;
	try {
		fs::path		v_path = path;
		fs::recursive_directory_iterator dir_it(v_path);
		uid_t			uid = 0;
		gid_t			gid = 0;
		constexpr mode_t	dir_mode = (S_IRUSR|S_IWUSR|
					    S_IRGRP|S_IWGRP|S_IROTH);
		constexpr mode_t	file_mode = (S_IRUSR|S_IWUSR|S_IRGRP|
					    S_IROTH);

		if (get_uid_and_gid(uid, gid) == -1) {
			throw std::runtime_error("failed to get uid/gid");
		} else if (chown(path, uid, gid) != 0) {
			std::string str("chown() error: ");
			str.append(strerror(errno));
			throw std::runtime_error(str);
		} else if (chmod(path, dir_mode) != 0) {
			std::string str("chmod() error: ");
			str.append(strerror(errno));
			throw std::runtime_error(str);
		}

		for (auto const &dir_ent : dir_it) {
			const std::string str(dir_ent.path().string());
			
			if (chown(str.c_str(), uid, gid) != 0) {
				warn("%s: chown(%s, ...)", __func__,
				    str.c_str());
			}

			if (dir_ent.is_directory()) {
				if (chmod(str.c_str(), dir_mode) != 0)
					warn("%s: chmod", __func__);
			} else if (dir_ent.is_regular_file()) {
				if (chmod(str.c_str(), file_mode) != 0)
					warn("%s: chmod", __func__);
			}
		}
	} catch (const std::exception &ex) {
		warnx("%s: %s", __func__, ex.what());
		return -1;
	}

	prep_done(path);
	return 0;
}
