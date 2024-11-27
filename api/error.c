/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file error.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2021, 2023-2024 NXP
*/

#include "genavb/error.h"

#define __ERR_NUM(err)	[GENAVB_ERR_NUM(err)]

static const char *genavb_GENERAL_error[] = {
	__ERR_NUM(GENAVB_SUCCESS) = "Success",
	__ERR_NUM(GENAVB_ERR_NO_MEMORY) = "Out of memory",
	__ERR_NUM(GENAVB_ERR_ALREADY_INITIALIZED) = "Library already initialized",
	__ERR_NUM(GENAVB_ERR_INVALID) = "Invalid parameters",
	__ERR_NUM(GENAVB_ERR_INVALID_PARAMS) = "Invalid library handle",
	__ERR_NUM(GENAVB_ERR_INVALID_PORT) = "Invalid port",
	__ERR_NUM(GENAVB_ERR_NOT_SUPPORTED) = "Feature not supported",
};

static const char *genavb_STREAM_error[] = {
	__ERR_NUM(GENAVB_ERR_STREAM_API_OPEN) = "Stream open error",
	__ERR_NUM(GENAVB_ERR_STREAM_BIND) = "Stream bind error",
	__ERR_NUM(GENAVB_ERR_STREAM_TX) = "Stream data write error",
	__ERR_NUM(GENAVB_ERR_STREAM_RX) = "Stream data read error",
	__ERR_NUM(GENAVB_ERR_STREAM_INVALID) = "Stream handle invalid",
	__ERR_NUM(GENAVB_ERR_STREAM_PARAMS) = "Invalid stream parameters",
	__ERR_NUM(GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA) = "Stream data not enough write error",
	__ERR_NUM(GENAVB_ERR_STREAM_NO_CALLBACK) = "Stream callback not set",
};

static const char *genavb_CTRL_error[] = {
	__ERR_NUM(GENAVB_ERR_CTRL_TRUNCATED) = "Control message truncated",
	__ERR_NUM(GENAVB_ERR_CTRL_INIT) = "Control init error",
	__ERR_NUM(GENAVB_ERR_CTRL_ALLOC) = "Control message allocation error",
	__ERR_NUM(GENAVB_ERR_CTRL_TX) = "Control message write error",
	__ERR_NUM(GENAVB_ERR_CTRL_RX) = "Control message read error",
	__ERR_NUM(GENAVB_ERR_CTRL_LEN) = "Invalid control message length",
	__ERR_NUM(GENAVB_ERR_CTRL_TIMEOUT) = "Control message timeout",
	__ERR_NUM(GENAVB_ERR_CTRL_INVALID) = "Control message incompatible with control channel",
	__ERR_NUM(GENAVB_ERR_CTRL_FAILED) = "Control command failed",
	__ERR_NUM(GENAVB_ERR_CTRL_UNKNOWN) = "Unknown control command",
	__ERR_NUM(GENAVB_ERR_STACK_NOT_READY) = "Stack not ready",
	__ERR_NUM(GENAVB_ERR_PTP_DOMAIN_INVALID) = "Invalid gPTP domain",
};

static const char *genavb_SOCKET_error[] = {
	__ERR_NUM(GENAVB_ERR_SOCKET_INIT) = "Socket open error",
	__ERR_NUM(GENAVB_ERR_SOCKET_PARAMS) = "Socket parameters invalid",
	__ERR_NUM(GENAVB_ERR_SOCKET_AGAIN) = "Socket no data available",
	__ERR_NUM(GENAVB_ERR_SOCKET_INVALID) = "Socket parameters invalid",
	__ERR_NUM(GENAVB_ERR_SOCKET_FAULT) = "Socket invalid address",
	__ERR_NUM(GENAVB_ERR_SOCKET_INTR) = "Socket blocking rx interrupted",
	__ERR_NUM(GENAVB_ERR_SOCKET_TX) = "Socket transmit error",
	__ERR_NUM(GENAVB_ERR_SOCKET_BUFLEN) = "Socket buffer length error",
};

static const char *genavb_CLOCK_error[] = {
	__ERR_NUM(GENAVB_ERR_CLOCK) = "Clock error",
};

static const char *genavb_TIMER_error[] = {
	__ERR_NUM(GENAVB_ERR_TIMER) = "Timer error",
};

static const char *genavb_ST_error[] = {
	__ERR_NUM(GENAVB_ERR_ST) = "Scheduled Traffic config error",
	__ERR_NUM(GENAVB_ERR_ST_NOT_SUPPORTED) = "Scheduled Traffic not supported error",
	__ERR_NUM(GENAVB_ERR_ST_HW_CONFIG) = "Scheduled Traffic hardware config error",
	__ERR_NUM(GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED) = "Scheduled Traffic max SDU config error",
};

static const char *genavb_SF_error[] = {
	__ERR_NUM(GENAVB_ERR_SF_NOT_SUPPORTED) = "Stream Filter not supported",
};

static const char *genavb_SG_error[] = {
	__ERR_NUM(GENAVB_ERR_SG_NOT_SUPPORTED) = "Stream Gate not supported",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_CYCLE_PARAMS) = "Stream Gate invalid cycle parameters",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_CYCLE_TIME) = "Stream Gate invalid cycle time",
	__ERR_NUM(GENAVB_ERR_SG_INVALID_BASETIME) = "Stream Gate invalid base time",
	__ERR_NUM(GENAVB_ERR_SG_GETTIME) = "Stream Gate gettime error",
	__ERR_NUM(GENAVB_ERR_SG_ADD) = "Stream Gate add entry error",
	__ERR_NUM(GENAVB_ERR_SG_UPDATE) = "Stream Gate update entry error",
	__ERR_NUM(GENAVB_ERR_SG_DELETE) = "Stream Gate delete entry error",
	__ERR_NUM(GENAVB_ERR_SG_MAX_ENTRIES) = "Stream Gate invalid entry id",
	__ERR_NUM(GENAVB_ERR_SG_ENTRY_NOT_FOUND) = "Stream Gate entry not found",
	__ERR_NUM(GENAVB_ERR_SG_GET_ENTRY) = "Stream Gate get entry error",
	__ERR_NUM(GENAVB_ERR_SG_GET_STATE_ENTRY) = "Stream Gate State get entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ADD) = "Stream Gate List add entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_DELETE_ADMIN) = "Stream Gate List delete admin entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_DELETE_OPER) = "Stream Gate List delete oper entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ENTRY_NOT_FOUND) = "Stream Gate List entry not found",
	__ERR_NUM(GENAVB_ERR_SG_LIST_GET_ADMIN_ENTRY) = "Stream Gate State get admin entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_GET_OPER_ENTRY) = "Stream Gate State get oper entry error",
	__ERR_NUM(GENAVB_ERR_SG_LIST_ENTRY_ALLOC_EID) = "Stream Gate List can not be allocated",
	__ERR_NUM(GENAVB_ERR_SG_RESET_IRX_OEX) = "Stream Gate reset IRX OEX error",
};

static const char *genavb_FM_error[] = {
	__ERR_NUM(GENAVB_ERR_FM_NOT_SUPPORTED) = "Flow Meter not supported",
	__ERR_NUM(GENAVB_ERR_FM_ADD) = "Flow Meter add entry error",
	__ERR_NUM(GENAVB_ERR_FM_UPDATE) = "Flow Meter update entry error",
	__ERR_NUM(GENAVB_ERR_FM_DELETE) = "Flow Meter delete entry error",
	__ERR_NUM(GENAVB_ERR_FM_MAX_ENTRIES) = "Flow Meter invalid entry id",
	__ERR_NUM(GENAVB_ERR_FM_ENTRY_NOT_FOUND) = "Flow Meter entry not found",
	__ERR_NUM(GENAVB_ERR_FM_RESET_MR) = "Flow Meter reset mark red error",
};

static const char *genavb_VLAN_error[] = {
	__ERR_NUM(GENAVB_ERR_VLAN_VID) = "Invalid VID number",
	__ERR_NUM(GENAVB_ERR_VLAN_CONTROL) = "Invalid VLAN control field",
	__ERR_NUM(GENAVB_ERR_VLAN_NOT_FOUND) = "VLAN entry not found",
	__ERR_NUM(GENAVB_ERR_VLAN_HW_CONFIG) = "VLAN hardware config error",
	__ERR_NUM(GENAVB_ERR_VLAN_DEFAULT_NOT_SUPPORTED) = "VLAN port default (PVID) not supported"
};

static const char *genavb_FRER_error[] = {
	__ERR_NUM(GENAVB_ERR_FRER_NOT_SUPPORTED) = "FRER not supported",
	__ERR_NUM(GENAVB_ERR_FRER_MAX_ENTRIES) = "FRER invalid entry id",
	__ERR_NUM(GENAVB_ERR_FRER_ENTRY_NOT_FOUND) = "FRER entry not found",
	__ERR_NUM(GENAVB_ERR_FRER_MAX_STREAM) = "FRER stream number invalid",
	__ERR_NUM(GENAVB_ERR_FRER_PARAMS) = "FRER parameters invalid",
	__ERR_NUM(GENAVB_ERR_FRER_ENTRY_USED) = "FRER entry already used",
	__ERR_NUM(GENAVB_ERR_FRER_HW_CONFIG) = "FRER hardware config error",
	__ERR_NUM(GENAVB_ERR_FRER_HW_READ) = "FRER hardware read error",
};

static const char *genavb_DSA_error[] = {
	__ERR_NUM(GENAVB_ERR_DSA_NOT_SUPPORTED) = "DSA not supported",
	__ERR_NUM(GENAVB_ERR_DSA_NOT_FOUND) = "DSA hardware table entry not found",
	__ERR_NUM(GENAVB_ERR_DSA_HW_CONFIG) = "DSA hardware table config error",
};

#define GENAVB_ERR_TYPE_STRINGS(type) \
	[GENAVB_ERR_TYPE_ ## type] = { \
		.str = genavb_ ## type ## _error, \
		.num_max = sizeof(genavb_ ## type ## _error), \
	}

static const struct  {
	const char **str;
	unsigned int num_max;
} genavb_error_type_strings[GENAVB_ERR_TYPE_MAX] = {
	GENAVB_ERR_TYPE_STRINGS(GENERAL),
	GENAVB_ERR_TYPE_STRINGS(STREAM),
	GENAVB_ERR_TYPE_STRINGS(CTRL),
	GENAVB_ERR_TYPE_STRINGS(SOCKET),
	GENAVB_ERR_TYPE_STRINGS(CLOCK),
	GENAVB_ERR_TYPE_STRINGS(TIMER),
	GENAVB_ERR_TYPE_STRINGS(ST),
	GENAVB_ERR_TYPE_STRINGS(SF),
	GENAVB_ERR_TYPE_STRINGS(SG),
	GENAVB_ERR_TYPE_STRINGS(FM),
	GENAVB_ERR_TYPE_STRINGS(VLAN),
	GENAVB_ERR_TYPE_STRINGS(FRER),
	GENAVB_ERR_TYPE_STRINGS(DSA),
};

const char *genavb_strerror(int error)
{
	unsigned int err, type, num;

	if (error < 0)
		err = -error;
	else
		err = error;

	type = GENAVB_ERR_TYPE(err);
	num = GENAVB_ERR_NUM(err);

	if ((type < GENAVB_ERR_TYPE_MAX) && (num < genavb_error_type_strings[type].num_max))
		return genavb_error_type_strings[type].str[num];
	else
		return "Unknown error code";
}
