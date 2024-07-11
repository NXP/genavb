/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Common 61883 IIDC functions
 @details
*/

#include "common/61883_iidc.h"

const avb_u32 avtp_61883_6_sampling_freq[IEC_61883_6_FDF_SFC_MAX + 1] = {
	[IEC_61883_6_FDF_SFC_32000] = 32000,
	[IEC_61883_6_FDF_SFC_44100] = 44100,
	[IEC_61883_6_FDF_SFC_48000] = 48000,
	[IEC_61883_6_FDF_SFC_88200] = 88200,
	[IEC_61883_6_FDF_SFC_96000] = 96000,
	[IEC_61883_6_FDF_SFC_176400] = 176400,
	[IEC_61883_6_FDF_SFC_192000] = 192000,
	[IEC_61883_6_FDF_SFC_RSVD] = 0,
};
