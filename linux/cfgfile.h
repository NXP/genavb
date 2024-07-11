/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Reads configuration from text file
 @details
*/

#ifndef _CFGFILE_H
#define _CFGFILE_H

#include "common/types.h"

#include "os_config.h"

struct _CFGENTRY {
	char *name;
	char *value;
	struct _CFGENTRY *next;
};

struct _SECTIONENTRY {
	char *secname;
	struct _CFGENTRY *key;
	struct _SECTIONENTRY *next;
};

int cfg_free_configtree(struct _SECTIONENTRY *configtree);
int cfg_get_string(struct _SECTIONENTRY *firstsec, const char *section,
		const char *key_name, const char *def_value, char *ret_value);
int cfg_get_string_list(struct _SECTIONENTRY *firstsec,
		const char *section, const char *key_name,
		const char *def_value, char (*item)[CFG_STRING_LIST_MAX_LEN], unsigned int len);
int cfg_get_uint(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const unsigned int def_value, const unsigned int min_value,
	const unsigned int max_value, unsigned int *ret_value);
int cfg_get_u32(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const u32 def_value, const u32 min_value,
	const u32 max_value, u32 *ret_value);
int cfg_get_ull(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const unsigned long long def_value, const unsigned long long min_value,
	const unsigned long long max_value, unsigned long long *ret_value);
int cfg_get_u64(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const u64 def_value, const u64 min_value,
	const u64 max_value, u64 *ret_value);
int cfg_get_signed_int(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const int def_value, const int min_value,
	const int max_value, int *ret_value);
int cfg_get_signed_int_list(struct _SECTIONENTRY *firstsec,
			    const char *section, const char *key_name,
			    const int *def_value, int item[], const unsigned int len);
int cfg_get_schar(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const signed char def_value, const signed char min_value,
	const signed char max_value, signed char *ret_value);
int cfg_get_ushort(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const unsigned short def_value, const unsigned short min_value,
	const unsigned short max_value, unsigned short *ret_value);
int cfg_get_uchar(struct _SECTIONENTRY *firstsec, const char *section, const char *key_name,
	const unsigned char def_value, const unsigned char min_value,
	const unsigned char max_value, unsigned char *ret_value);

struct _SECTIONENTRY *cfg_read (const char *filename);

#define SRP_CONF_FILENAME	"/etc/genavb/srp.cfg"
int cfg_get_sr_class_list(struct _SECTIONENTRY *configtree, uint8_t *_sr_class);

int cfg_get_net_mode(struct _SECTIONENTRY *configtree, const char *config_name, const char *default_mode, network_mode_t *net_mode_config, unsigned int *enabled_modes_flag);
#endif /* _CFGFILE_H */
