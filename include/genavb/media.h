/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2019, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file media.h
 \brief GenAVB public API
 \details media header definitions.
 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018-2019, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_MEDIA_H_
#define _GENAVB_PUBLIC_MEDIA_H_

#include "types.h"
#include "net_types.h"

#define MEDIA_TS_PER_PACKET	7

struct media_desc {
	NET_DESC_COMMON;

	avb_u8 n_ts;			/**< number of valid timestamps in the array below*/
	avb_u32 bytes_lost;		/**< Number of bytes lost between previous packet and current */
	struct {
		avb_u32 val;
		avb_u16 flags;
		avb_u16 offset;
	} avtp_ts[MEDIA_TS_PER_PACKET];	/**< array of AVTP timestamps (in gPTP time), their offsets in bytes, starting from the start of AVTP payload data and
					* AVTP_TIMESTAMP_* flags (from include/avtp.h). */
};

#define AVTP_FLAGS_TO_MEDIA_DESC(x)		((x))
#define MEDIA_DESC_FLAGS_TO_AVTP(x)		((x))

struct media_rx_desc {
	struct net_tx_desc net;

	avb_u8 ts_n;
	struct {
		avb_u32 val;	/**< AVTP timestamps (in gPTP time) */
		avb_u16 offset;	/**< offsets in bytes for the timestamps, starting from the start of AVTP payload data. */
	} avtp_ts[MEDIA_TS_PER_PACKET];
};

#endif /* _GENAVB_PUBLIC_MEDIA_H_ */
