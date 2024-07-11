/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Common AAF functions
 @details
*/

#include "common/aaf.h"

const avb_u32 avtp_aaf_sampling_freq[AAF_NSR_MAX + 1] = {
	[AAF_NSR_USER_SPECIFIED] = 48000, /* FIXME */
	[AAF_NSR_8000] = 8000,
	[AAF_NSR_16000] = 16000,
	[AAF_NSR_32000] = 32000,
	[AAF_NSR_44100] = 44100,
	[AAF_NSR_48000] = 48000,
	[AAF_NSR_88200] = 88200,
	[AAF_NSR_96000] = 96000,
	[AAF_NSR_176400] = 176400,
	[AAF_NSR_192000] = 192000,
	[AAF_NSR_24000] = 24000,
	[AAF_NSR_RESERVED1] = 0,
	[AAF_NSR_RESERVED2] = 0,
	[AAF_NSR_RESERVED3] = 0,
	[AAF_NSR_RESERVED4] = 0,
	[AAF_NSR_RESERVED5] = 0
};
