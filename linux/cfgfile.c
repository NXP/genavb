/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Reads configuration from text file
 @details
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "cfgfile.h"
#include "common/log.h"
#include "common/types.h"
#include "genavb/helpers.h"
#include "genavb/sr_class.h"

/* maximum size of a config file line  */
#define LINE_MAX_SIZE 512


/** Frees the memory allocated for the configtree, with cfg_read() function.
 * \return		0 on success.
 * \param configtree	pointer to the configtree
 */
int cfg_free_configtree(struct _SECTIONENTRY *configtree)
{
	struct _SECTIONENTRY *currsec;
	struct _CFGENTRY *currcfg;

	while ((currsec = configtree) != NULL) { /* loop to free the _SECTIONENTRY linked list */
		while ((currcfg = currsec->key) != NULL) { /* loop to free the _CFGENTRY linked list */
			currsec->key = currsec->key->next;
			if (currcfg->name != NULL)
				free (currcfg->name);
			if (currcfg->value != NULL)
				free (currcfg->value);
			free (currcfg);
		}

		configtree = configtree->next;
		free (currsec->secname);
		free (currsec);
	}

	os_log(LOG_DEBUG, "Config file parser: successfully freed configtree memory\n");

	return 0;
}


static struct _CFGENTRY *cfg_get_section(struct _SECTIONENTRY *secnode, const char *section)
{
	while (secnode) {
		if (!strcmp (secnode->secname, section)) {
			os_log(LOG_DEBUG, "secnode->secname=%s, section=%s, secnode->key=%p\n", secnode->secname, section, secnode->key);
			return secnode->key;
		}
		secnode = secnode->next;
	}

	return NULL;
}


static char *cfg_get_val_with_key(struct _CFGENTRY *cfgnode, const char *key_name)
{
	while (cfgnode) {
		if (!strcmp (cfgnode->name, key_name)) {
			os_log(LOG_DEBUG, "cfgnode->name=%s, key_name=%s, cfgnode->value=%s\n", cfgnode->name, key_name, cfgnode->value);
			return cfgnode->value;
		}
		cfgnode = cfgnode->next;
	}

	os_log(LOG_DEBUG, "Config file parser: key '%s' not found in cfg file\n", key_name);

	return NULL;
}


/** Fetch a string value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	default value that will be returned in ret_value if section or key_name is not found. Can be NULL (input)
 * \param ret_value	pointer to string value corresponding to key_name (output)
 */
int cfg_get_string(struct _SECTIONENTRY *firstsec,
		const char *section, const char *key_name,
		const char *def_value, char *ret_value)
{
	struct _CFGENTRY *cfg;
	char *read_value;

	os_log(LOG_DEBUG, "IN\n");

	cfg = cfg_get_section(firstsec, section);
	if (cfg == NULL) { /* section does not exist */
		if (def_value != NULL) { /* use default value if provided */
			h_strncpy(ret_value, def_value, CFG_STRING_MAX_LEN);
			os_log(LOG_INFO, "Warning: Config file parser: section [%s] not found in cfg file, "
				"using default (%s = %s)\n", section, key_name, def_value);
			goto used_default;
		} else { /* section does not exist, and no default value is provided */
			os_log(LOG_ERR, "Config file parser: section [%s] not found in cfg file, "
				"and no default value available for key '%s'.\n", section, key_name);
			goto exit;
		}
	}

	read_value = cfg_get_val_with_key(cfg, key_name);
	if (read_value != NULL) { /* value is available from cfg file */
		if (strlen(read_value) < CFG_STRING_MAX_LEN) {
			h_strncpy(ret_value, read_value, CFG_STRING_MAX_LEN);
			os_log(LOG_INFO, "Config file parser: in section [%s], key '%s', value is '%s'\n", section, key_name, ret_value);
		} else {
			os_log(LOG_ERR, "Config file parser: for key '%s', value too long (>255)!\n", key_name);
			goto exit;
		}
	} else if (def_value != NULL) { /* use default value if available */
		h_strncpy(ret_value, def_value, CFG_STRING_MAX_LEN);
		os_log(LOG_INFO, "Warning: Config file parser: key '%s' not found in section [%s]. Using default '%s'.\n",
				key_name, section, def_value);
	} else { /* not found in cfg file and no default value */
		os_log(LOG_ERR, "Config file parser: key '%s' not found in section [%s]. And no default value available.\n",
				key_name, section);
		goto exit;
	}

used_default:
	return 0;

exit:
	return -1;
}

static char *cfg_string_trim(char *string)
{
	char *pstr, *pstr2;

	/* remove leading spaces and tabs */
	pstr = string;
	while ((*pstr == ' ') || (*pstr == '\t'))
		pstr++;

	/* remove trailing spaces, tabs and newlines */
	pstr2 = pstr + strlen (pstr) - 1;
	while ((*pstr2 == ' ') || (*pstr2 == '\t') || (*pstr2 == '\n'))
		*pstr2-- = 0;

	return pstr;
}

int cfg_get_string_list(struct _SECTIONENTRY *firstsec,
		const char *section, const char *key_name,
		const char *def_value, char (*item)[CFG_STRING_LIST_MAX_LEN], unsigned int len)
{
	char stringvalue[CFG_STRING_MAX_LEN];
	unsigned int i = 0;
	char *string;

	if (cfg_get_string(firstsec, section, key_name, def_value, stringvalue)) {
		goto err;
	}

	string = strtok(stringvalue, ",");
	while (string && (i < len)) {
		string = cfg_string_trim(string);

		h_strncpy(item[i], string, CFG_STRING_LIST_MAX_LEN);
		i++;
		string = strtok(NULL, ",");
	}

	return i;

err:
	return -1;
}

/** Fetch a unsigned int value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	uint min value the ret_value can take (input)
 * \param max_value	uint max value the ret_value can take (input)
 * \param ret_value	pointer to uint value corresponding to key_name (output)
 */
int cfg_get_uint(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const unsigned int def_value, const unsigned int min_value,
	const unsigned int max_value, unsigned int *ret_value)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	char stringdefault[32] = "";
	char * endptr;

	/* verify params consistency */
	if (min_value > def_value || max_value < def_value) {
		os_log(LOG_ERR, "AVB cfg file: invalid min/max parameters for key '%s' in section [%s]\n", key_name, section);
		goto exit;
	}

	snprintf(stringdefault, 32, "%u", def_value);

	if (!cfg_get_string(firstsec, section, key_name, stringdefault, stringvalue)) { /* fetch value from cfg file */
		errno = 0;
		*ret_value = strtoul(stringvalue, &endptr, 0);

		if (errno) {
			os_log(LOG_ERR, "AVB cfg file: Error reading '%s' value in [%s] section. Not a valid unsigned integer\n", key_name, section);
			goto exit;
		}
		if (errno || (*endptr != '\0')) { /* over/underflow occured OR did not convert the entire string */
			os_log(LOG_ERR, "AVB cfg file: Error reading '%s' value in [%s] section. Not a valid unsigned int. Using default value '%u'\n", key_name, section, def_value);
			*ret_value = def_value;
		}

		if (*ret_value > max_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%u/%u). Forcing to '%u'.\n",
				key_name, min_value, max_value, max_value);
			*ret_value = max_value;
		} else if (*ret_value < min_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%u/%u). Forcing to '%u'.\n",
				key_name, min_value, max_value, min_value);
			*ret_value = min_value;
		}

	} else { /* use default value */
		*ret_value = def_value;
		os_log(LOG_INFO, "Warning: Config file parser: key '%s' not found in section [%s]. Using default '%u'.\n",
				key_name, section, def_value);
	}

	return 0;

exit:
	return -1;

}


/** Fetch a u32 value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	u32 min value the ret_value can take (input)
 * \param max_value	u32 max value the ret_value can take (input)
 * \param ret_value	pointer to u32 value corresponding to key_name (output)
 */
int cfg_get_u32(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const u32 def_value, const u32 min_value,
	const u32 max_value, u32 *ret_value)
{
	int rc;
	unsigned int __ret_value = 0;

	rc = cfg_get_uint(firstsec, section, key_name, def_value, min_value, max_value, &__ret_value);
	if (rc < 0)
		goto exit;

	*ret_value = __ret_value;

exit:
	return rc;
}


/** Fetch a unsigned long long value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	unsigned long long default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	unsigned long long min value the ret_value can take (input)
 * \param max_value	unsigned long long max value the ret_value can take (input)
 * \param ret_value	pointer to unsigned long long value corresponding to key_name (output)
 */
int cfg_get_ull(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const unsigned long long def_value, const unsigned long long min_value,
	const unsigned long long max_value, unsigned long long *ret_value)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	char stringdefault[32] = "";
	char * endptr;

	/* verify params consistency */
	if (min_value > def_value || max_value < def_value) {
		os_log(LOG_ERR, "AVB cfg file: invalid min/max parameters for key '%s' in section [%s]\n", key_name, section);
		goto exit;
	}

	snprintf(stringdefault, 32, "%llu", def_value);

	if (!cfg_get_string(firstsec, section, key_name, stringdefault, stringvalue)) { /* fetch value from cfg file */
		errno = 0;
		*ret_value = strtoull(stringvalue, &endptr, 0);

		if (errno || (*endptr != '\0')) { /* over/underflow occured OR did not convert the entire string */
			os_log(LOG_ERR, "AVB cfg file: Error reading '%s' value in [%s] section. Not a valid unsigned long long. Using default value '%llu'\n", key_name, section, def_value);
			*ret_value = def_value;
		}

		if (*ret_value > max_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%llu/%llu). Forcing to '%llu'.\n",
				key_name, min_value, max_value, max_value);
			*ret_value = max_value;
		} else if (*ret_value < min_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%llu/%llu). Forcing to '%llu'.\n",
				key_name, min_value, max_value, min_value);
			*ret_value = min_value;
		}


	} else { /* use default value */
		*ret_value = def_value;
		os_log(LOG_INFO, "Warning: Config file parser: key '%s' not found in section [%s]. Using default '%llu'.\n",
				key_name, section, def_value);
	}

	return 0;

exit:
	return -1;

}


/** Fetch a u64 value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	u64 default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	u64 min value the ret_value can take (input)
 * \param max_value	u64 max value the ret_value can take (input)
 * \param ret_value	pointer to u64 value corresponding to key_name (output)
 */
int cfg_get_u64(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const u64 def_value, const u64 min_value,
	const u64 max_value, u64 *ret_value)
{
	unsigned long long __ret_value = 0;
	int rc;

	rc = cfg_get_ull(firstsec, section, key_name, def_value, min_value, max_value, &__ret_value);
	if (rc < 0)
		goto exit;

	*ret_value = __ret_value;

exit:
	return rc;
}


/** Fetch a signed int value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	int default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	int value the ret_value can take (input)
 * \param max_value	int max value the ret_value can take (input)
 * \param ret_value	pointer to int value corresponding to key_name (output)
 */
int cfg_get_signed_int(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const int def_value, const int min_value,
	const int max_value, int *ret_value)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	char stringdefault[32] = "";
	char * endptr;

	/* verify params consistency */
	if (min_value > def_value || max_value < def_value) {
		os_log(LOG_ERR, "AVB cfg file: invalid min/max parameters for key '%s' in section [%s]\n", key_name, section);
		goto exit;
	}

	snprintf(stringdefault, 32, "%d", def_value);

	if (!cfg_get_string(firstsec, section, key_name, stringdefault, stringvalue)) { /* fetch value from cfg file */
		errno = 0;
		*ret_value = strtol(stringvalue, &endptr, 0);

		if (errno || (*endptr != '\0')) { /* over/underflow occured OR did not convert the entire string */
			os_log(LOG_ERR, "AVB cfg file: Error reading '%s' value in [%s] section. Not a valid signed int. Using default value '%d'\n", key_name, section, def_value);
			*ret_value = def_value;
		}

		if (*ret_value > max_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%d/%d). Forcing to '%d'.\n",
				key_name, min_value, max_value, max_value);
			*ret_value = max_value;
		} else if (*ret_value < min_value) {
			os_log(LOG_INFO, "Warning: Config file parser: key '%s' value is out of range (%d/%d). Forcing to '%d'.\n",
				key_name, min_value, max_value, min_value);
			*ret_value = min_value;
		}

	} else { /* use default value */
		*ret_value = def_value;
		os_log(LOG_INFO, "Warning: Config file parser: key '%s' not found in section [%s]. Using default '%d'.\n",
				key_name, section, def_value);
	}

	return 0;

exit:
	return -1;

}

static unsigned int parse_list_int(char *str, int *list, const unsigned int nb_elem)
{
	unsigned int i = 0;
	char *string;
	char *endptr;
	int temp;

	string = strtok(str, ",");
	while (string && (i < nb_elem)) {
		errno = 0;
		temp = strtol(string, &endptr, 0);
		if (errno != 0)
			break;
		if (endptr != string)
			list[i] = temp;
		i++;
		string = strtok(NULL, ",");
	}

	return i;
}

/**
 * Fetch a list of signed int values corresponding to a key, inside a
 * configuration tree previously filled with cfg_read().
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	list of int default value that will be returned in
 *                      ret_value if key_name is not found (input)
 * \param ret_value	pointer to the list of int values corresponding to key_name (output)
 * \param len		number of items present in the def_value list (input)
 */
int cfg_get_signed_int_list(struct _SECTIONENTRY *firstsec,
			    const char *section, const char *key_name,
			    const int *def_value, int ret_value[], const unsigned int len)
{
	int i, rc;
	char stringvalue[CFG_STRING_MAX_LEN] = "";

	/* Initialize ret_value with def_value */
	for (i = 0; i < len; i++)
		ret_value[i] = def_value[i];

	/* Overwrite config from file if entry is present */
	rc = cfg_get_string(firstsec, section, key_name, "", stringvalue);
	if (!rc) {
		if (!parse_list_int(stringvalue, ret_value, len))
			os_log(LOG_INFO,
			       "Warning: Config file parser: key '%s' not found in section [%s]. Using default.\n",
			       key_name, section);
	}

	return rc;
}

/** Fetch an unsigned short value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	unsigned short default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	unsigned short min value the ret_value can take (input)
 * \param max_value	unsigned short max value the ret_value can take (input)
 * \param ret_value	pointer to unsigned short value corresponding to key_name (output)
 */
int cfg_get_ushort(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const unsigned short def_value, const unsigned short min_value,
	const unsigned short max_value, unsigned short *ret_value)
{
	unsigned int __ret_value = 0;
	int rc;

	rc = cfg_get_uint(firstsec, section, key_name, def_value, min_value, max_value, &__ret_value);
	if (rc < 0)
		goto exit;

	*ret_value = __ret_value;

exit:
	return rc;
}

/** Fetch an unsigned char value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	unsigned char default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	unsigned char min value the ret_value can take (input)
 * \param max_value	unsigned char max value the ret_value can take (input)
 * \param ret_value	pointer to unsigned char value corresponding to key_name (output)
 */
int cfg_get_uchar(struct _SECTIONENTRY *firstsec,
	const char *section, const char *key_name,
	const unsigned char def_value, const unsigned char min_value,
	const unsigned char max_value, unsigned char *ret_value)
{
	unsigned int __ret_value = 0;
	int rc;

	rc = cfg_get_uint(firstsec, section, key_name, def_value, min_value, max_value, &__ret_value);
	if (rc < 0)
		goto exit;

	*ret_value = __ret_value;

exit:
	return rc;
}

/** Fetch a signed char value corresponding to a key, inside a configuration tree previously filled with cfg_read().
 * Limits the fetched value to the range between min_value and max_value
 * Returns def_value if key_name is not found in cfg file
 * \return		0 if OK, or -1 in case of error.
 * \param firstsec	pointer to config tree (input)
 * \param section	pointer to section of the config tree to look into (input)
 * \param key_name	pointer to key name inside section (input)
 * \param def_value	signed char default value that will be returned in ret_value if key_name is not found (input)
 * \param min_value	signed char  value the ret_value can take (input)
 * \param max_value	signed char  max value the ret_value can take (input)
 * \param ret_value	pointer to signed char  value corresponding to key_name (output)
 */
int cfg_get_schar(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const signed char def_value, const signed char min_value,
	const signed char max_value, signed char *ret_value)
{
	signed int __ret_value = 0;
	int rc;

	rc = cfg_get_signed_int(firstsec, section, key_name, def_value, min_value, max_value, &__ret_value);
	if(rc < 0)
		goto exit;

	*ret_value = __ret_value;
exit:
	return rc;
}

/** Reads the config file and builds a double linked-list made of sections, and key/value pairs.
 * The configtree filled up with this function must be freed by calling cfg_free_configtree() function
 * \return		pointer to the configuration tree.
 * \param filename	path to the cfg file to read from (input)
 */
struct _SECTIONENTRY *cfg_read (const char *filename)
{
	FILE *cf;
	struct _SECTIONENTRY *conf;
	struct _SECTIONENTRY *secnode, *slast;
	struct _CFGENTRY *cfgnode, *clast;
	char *line = NULL, *pstr, *pstr2, *pstr3;
	unsigned int num;
	size_t len = 0;
	ssize_t nread;

	conf = NULL;
	secnode = slast = NULL;
	cfgnode = clast = NULL;

	cf = fopen (filename, "r");
	if (!cf)
		goto err_file;

	num = 0;

	while ((nread = getline(&line, &len, cf)) != -1)
	{
		if (nread != strlen(line)) {
			os_log(LOG_ERR, "Unexpected embedded null byte(s)\n");
			goto out;
		}

		num++;
		pstr = line;

		if (*pstr == '#') /* discard commented lines */
			continue;

		pstr = strchr (pstr, '#');
		if (pstr)
			*pstr = 0;

		pstr = line;
		while ((*pstr == ' ') || (*pstr == '\t'))
			pstr++;

		pstr2 = pstr + strlen (pstr) - 1;
		while ((*pstr2 == ' ') || (*pstr2 == '\t') || (*pstr2 == '\n'))
			*pstr2-- = 0;

		if (!*pstr)	/* ignore blanks */
			continue;

		if (*pstr == '[') { /* look for sections */
			pstr++;

			pstr2 = strchr (pstr, ']');

			if (!pstr2) {
				os_log(LOG_ERR, "Missing closing ] on line #%d.\n", num);
				goto out;
			}

			if (*(pstr2 + 1)) {
				os_log(LOG_ERR, "Unexpected characters after ] on line #%d.\n", num);
				goto out;
			}

			*pstr2-- = 0;

			while ((*pstr2 == ' ') || (*pstr2 == '\t'))
				*pstr2-- = 0;

			if (!*pstr) {
				os_log(LOG_ERR, "Missing section name on line #%d.\n", num);
				goto out;
			}

			for (secnode = conf; secnode; secnode = secnode->next)
				if (strcasecmp (secnode->secname, pstr) == 0) {
					os_log(LOG_ERR, "Duplicate section values (%s) in line #%d.\n", secnode->secname, num);
					goto out;
				}

			secnode = (struct _SECTIONENTRY *) malloc (sizeof (struct _SECTIONENTRY));
			if (!secnode)
				goto err_alloc;

			memset (secnode, 0, sizeof (struct _SECTIONENTRY));

			secnode->secname = (char *) malloc (strlen (pstr) + 1);
			if (!secnode->secname) {
				free(secnode);
				goto err_alloc;
			}

			h_strncpy(secnode->secname, pstr, (strlen (pstr) + 1));

			if (!slast)
				conf = slast = secnode;
			else {
				slast->next = secnode;
				slast = secnode;
			}

			clast = cfgnode = NULL;

		} else if ((pstr2 = strchr (pstr, '=')) != NULL) { /* look for key/value pairs */

			if (!secnode) { /* key/value pairs outside of section */
				os_log(LOG_ERR, "No [SECTION] value specified by line #%d.\n", num);
				goto out;
			}

			pstr3 = pstr2 - 1;
			while ((*pstr3 == ' ') || (*pstr3 == '\t'))
				*pstr3-- = 0;

			*pstr2++ = 0;
			while ((*pstr2 == ' ') || (*pstr2 == '\t'))
				pstr2++;

			if (!*pstr) {
				os_log(LOG_ERR, "Missing KEY on line #%d.\n", num);
				goto out;
			}

			if (!*pstr2) {
				os_log(LOG_ERR, "Missing VALUE on line #%d.\n", num);
				goto out;
			}

			for (cfgnode = secnode->key; cfgnode; cfgnode = cfgnode->next)
				if (strcasecmp (cfgnode->name, pstr) == 0) {
					os_log(LOG_ERR, "Duplicate KEY on line #%d.\n", num);
					goto out;
				}

			cfgnode = (struct _CFGENTRY *) malloc (sizeof (struct _CFGENTRY));
			if (!cfgnode)
				goto err_alloc;

			memset (cfgnode, 0, sizeof (struct _CFGENTRY));

			cfgnode->name = (char *) malloc (strlen (pstr) + 1);
			if (!cfgnode->name) {
				free(cfgnode);
				goto err_alloc;
			}

			h_strncpy(cfgnode->name, pstr, (strlen (pstr) + 1));

			cfgnode->value = (char *) malloc (strlen (pstr2) + 1);
			if (!cfgnode->value) {
				free(cfgnode->name);
				free(cfgnode);
				goto err_alloc;
			}

			h_strncpy(cfgnode->value, pstr2, (strlen (pstr2) + 1));

			if (!clast)
				secnode->key = clast = cfgnode;
			else {
				clast->next = cfgnode;
				clast = cfgnode;
			}
		} else {
			os_log(LOG_ERR, "Invalid config line #%d.\n", num);
			goto out;
		}
	}

	free(line);
	fclose (cf);

	return (conf);


err_alloc:
	os_log(LOG_ERR, "Memory allocation error!\n");

out:
	if (conf)
		cfg_free_configtree(conf);

	free(line);
	fclose (cf);
	return (NULL);

err_file:
	os_log(LOG_ERR, "Config file parser: Can't open config file \"%s\"\n", filename);
	return (NULL);
}

int cfg_get_sr_class_list(struct _SECTIONENTRY *configtree, uint8_t *_sr_class)
{
	int i, n;
	uint8_t sr_class[CFG_SR_CLASS_MAX] = {
		[0 ... CFG_SR_CLASS_MAX - 1] = SR_CLASS_NONE
	};
	char item[CFG_SR_CLASS_MAX][CFG_STRING_LIST_MAX_LEN];

	n = cfg_get_string_list(configtree, "SRP_GENERAL", "sr_class_enabled", "A, B", item, CFG_SR_CLASS_MAX);
	if (n < 0) {
		os_log(LOG_ERR, "Failed to read sr_class_enabled value\n");
		goto err;
	}

	for (i = 0; i < n; i++)
		sr_class[i] = str_to_sr_class(item[i]);

	memcpy(_sr_class, &sr_class, sizeof(sr_class));

	return 0;

err:
	return -1;
}

int cfg_get_net_mode(struct _SECTIONENTRY *configtree, const char *config_name, const char *default_mode, network_mode_t *net_mode_config, unsigned int *enabled_modes_flag)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	int rc = 0;

	if (!net_mode_config || !enabled_modes_flag) {
		rc = -1;
		goto exit;
	}

	if (cfg_get_string(configtree, "NET_MODES", config_name, default_mode, stringvalue)) {
		rc = -1;
		goto exit;
	}

	if (!strcmp(stringvalue, "std"))
		*net_mode_config = NET_STD;
	else if (!strcmp(stringvalue, "xdp"))
		*net_mode_config = NET_XDP;
	else if (!strcmp(stringvalue, "avb"))
		*net_mode_config = NET_AVB;
	else {
		os_log(LOG_ERR, "System cfg file error: unsupported %s (%s)\n", config_name, stringvalue);
		rc = -1;
		goto exit;
	}

exit:
	return rc;
}
