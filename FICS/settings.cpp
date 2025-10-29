// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <err.h>
#include <errno.h>

#include <string>
#include <vector>

#include "copyfile.h"
#include "interpreter.h"
#include "settings.h"

struct setting {
	std::string		name;
	enum setting_type	type;
	std::string		value;

	setting()
	{
		this->name.assign("");
		this->type = STYPE_STRING;
		this->value.assign("");
	}

	setting(const char *p_name, enum setting_type p_type,
		const char *p_value)
	{
		this->name.assign(p_name);
		this->type = p_type;
		this->value.assign(p_value);
	}
};

static std::vector<setting> settings;

#if defined(__cplusplus) && __cplusplus >= 201103L
#define SET_PB(obj) settings.emplace_back(obj)
#else
#define SET_PB(obj) settings.push_back(obj)
#endif

void
settings_init(void)
{
	struct setting s1("server_hostname", STYPE_STRING, "");
	struct setting s2("server_name",     STYPE_STRING, "Xfics");
	struct setting s3("server_location", STYPE_STRING, "");
	struct setting s4("HADMINHANDLE",    STYPE_STRING, "");
	struct setting s5("HADMINEMAIL",     STYPE_STRING, "");
	struct setting s6("REGMAIL",         STYPE_STRING, "");
	struct setting s7("privdrop_user",   STYPE_STRING, "nobody");
	struct setting s8("sysgroup",        STYPE_STRING, "chess");

	SET_PB(s1);
	SET_PB(s2);
	SET_PB(s3);
	SET_PB(s4);
	SET_PB(s5);
	SET_PB(s6);
	SET_PB(s7);
	SET_PB(s8);
}

void
settings_deinit(void)
{
	fprintf(stderr, "FICS: Deinitialized the conf settings\n");
}

static bool
is_recognized_setting(const char *name)
{
	if (name == nullptr || strings_match(name, ""))
		return false;
	for (auto it = settings.begin(); it != settings.end(); ++it) {
		if (strings_match((*it).name.c_str(), name))
			return true;
	}
	return false;
}

static int
install_setting(const char *name, const char *value)
{
	if (name == nullptr || value == nullptr)
		return EINVAL;
	for (auto it = settings.begin(); it != settings.end(); ++it) {
		if (strings_match((*it).name.c_str(), name)) {
			(*it).value.assign(value);
			return 0;
		}
	}
	return ENOENT;
}

const char *
settings_get(const char *set_name)
{
	if (set_name == nullptr || strings_match(set_name, ""))
		return ("");
	for (auto it = settings.begin(); it != settings.end(); ++it) {
		if (strings_match((*it).name.c_str(), set_name))
			return (*it).value.c_str();
	}
	return ("");
}

void
settings_read_conf(const char *path)
{
	FILE *fp = nullptr;

	if (!is_regular_file(path)) {
		errx(1, "%s: either the config file is nonexistent"
		    "  --  or it isn't a regular file", __func__);
	} else if ((fp = fopen(path, "r")) == nullptr) {
		err(1, "%s: fopen", __func__);
	}

	Interpreter_processAllLines(fp, path, is_recognized_setting,
	    install_setting);

	if (feof(fp))
		fclose(fp);
	else if (ferror(fp))
		errx(1, "%s: %s", __func__, g_fgets_nullret_err1);
	else
		errx(1, "%s: %s", __func__, g_fgets_nullret_err2);
}
