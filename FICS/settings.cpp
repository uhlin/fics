// SPDX-FileCopyrightText: 2025 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <err.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <vector>

#include "addgroup.h"
#include "copyfile.h"
#include "interpreter.h"
#include "settings.h"

#if __linux__
#include <bsd/string.h>
#endif

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

static bool
is_setting_ok(const char *value, enum setting_type type)
{
	if (value == nullptr)
		return false;

	switch (type) {
	case STYPE_BOOLEAN: {
		if (!strings_match(value, "yes") &&
		    !strings_match(value, "YES") &&
		    !strings_match(value, "no") &&
		    !strings_match(value, "NO")) {
			warnx("%s: booleans must be either: "
			    "yes, YES, no or NO", __func__);
			return false;
		}
		break;
	}
	case STYPE_INTEGER: {
		if (!is_numeric(value)) {
			warnx("%s: integer not all numeric", __func__);
			return false;
		}
		break;
	}
	case STYPE_STRING: {
		if (strpbrk(value, "\f\n\r\t\v\"") != nullptr) {
			warnx("%s: illegal characters in string", __func__);
			return false;
		}
		break;
	}
	default:
		errx(1, "%s: statement reached unexpectedly", __func__);
		break;
	}

	return true;
}

static int
install_setting(const char *name, const char *value)
{
	if (name == nullptr || value == nullptr)
		return EINVAL;
	for (auto it = settings.begin(); it != settings.end(); ++it) {
		if (strings_match((*it).name.c_str(), name)) {
			if (!is_setting_ok(value, (*it).type))
				return EINVAL;
			(*it).value.assign(value);
			return 0;
		}
	}
	return ENOENT;
}

void
check_some_settings_strictly(void)
{
	err_reason_t reason;

	if (!is_valid_hostname(settings_get("server_hostname"), &reason))
		errx(1, "error: server_hostname: %s", reason.data);
	else if (!is_valid_username(settings_get("privdrop_user"), &reason))
		errx(1, "error: privdrop_user: %s", reason.data);
	else if (!is_valid_group_name(settings_get("sysgroup")))
		errx(1, "error: sysgroup: invalid group name");
}

bool
is_numeric(const char *string)
{
	if (string == nullptr || strcmp(string, "") == 0)
		return false;

	for (const char *cp = &string[0]; *cp != '\0'; cp++) {
		if (!isdigit(*cp))
			return false;
	}

	return true;
}

bool
is_valid_hostname(const char *p_str, err_reason_t *p_reason)
{
	const char	legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789-.";
	const size_t	HOST_MIN = 3;
	const size_t	HOST_MAX = 253;
	size_t		len = 0;

	if (p_str == nullptr || strcmp(p_str, "") == 0) {
		strlcpy(p_reason->data, "no hostname", sizeof p_reason->data);
		return false;
	} else if ((len = strlen(p_str)) < HOST_MIN) {
		snprintf(p_reason->data, sizeof p_reason->data, "hostname too "
		    "short (%zu): min=%zu", len, HOST_MIN);
		return false;
	} else if (len > HOST_MAX) {
		snprintf(p_reason->data, sizeof p_reason->data, "hostname too "
		    "long (%zu): max=%zu", len, HOST_MAX);
		return false;
	} else if (strstr(p_str, "..")) {
		strlcpy(p_reason->data, "dot followed by dot",
		    sizeof p_reason->data);
		return false;
	}

	for (const char *cp = p_str; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == nullptr) {
			snprintf(p_reason->data, sizeof p_reason->data,
			    "invalid chars found: first was '%c'", *cp);
			return false;
		}
	}

	return true;
}

bool
is_valid_username(const char *p_str, err_reason_t *p_reason)
{
	const char	legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789_-";
	const size_t	USER_MIN = 2;
	const size_t	USER_MAX = 30;
	size_t		len = 0;

	if (p_str == nullptr || strcmp(p_str, "") == 0) {
		strlcpy(p_reason->data, "no username", sizeof p_reason->data);
		return false;
	} else if ((len = strlen(p_str)) < USER_MIN) {
		snprintf(p_reason->data, sizeof p_reason->data, "username too "
		    "short (%zu): min=%zu", len, USER_MIN);
		return false;
	} else if (len > USER_MAX) {
		snprintf(p_reason->data, sizeof p_reason->data, "username too "
		    "long (%zu): max=%zu", len, USER_MAX);
		return false;
	}

	for (const char *cp = p_str; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == nullptr) {
			snprintf(p_reason->data, sizeof p_reason->data,
			    "invalid chars found: first was '%c'", *cp);
			return false;
		}
	}

	return true;
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

	if (path == nullptr)
		errx(1, "%s: null path", __func__);
	else if ((fp = fopen(path, "r")) == nullptr)
		err(1, "%s: fopen", __func__);

	Interpreter_processAllLines(fp, path, is_recognized_setting,
	    install_setting);

	if (feof(fp))
		fclose(fp);
	else if (ferror(fp))
		errx(1, "%s: %s", __func__, g_fgets_nullret_err1);
	else
		errx(1, "%s: %s", __func__, g_fgets_nullret_err2);
}
