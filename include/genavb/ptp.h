/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2019-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file ptp.h
 \brief GenAVB public API
 \details ptp header definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2019-2021, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_PTP_H_
#define _GENAVB_PUBLIC_PTP_H_

#include "types.h"

/**
 * GM Clock Identity (802.1AS - Table 10.7)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_clock_identity {
	avb_u8 identity[8];
};

/**
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_port_identity {
	avb_u8 clock_identity[8];
	avb_u16 port_number;
};

/**
 * PTP Message Header (802.1AS - Table 10.4)
 * \ingroup protocol
 */
struct __attribute__ ((packed)) ptp_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 transport_specific:4;
	avb_u8 msg_type:4;

	avb_u8 minor_version_ptp:4;
	avb_u8 version_ptp:4;
#else
	avb_u8 msg_type:4;
	avb_u8 transport_specific:4;

	avb_u8 version_ptp:4;
	avb_u8 minor_version_ptp:4;
#endif

	avb_u16 msg_length;
	avb_u8 domain_number;
	avb_u8 reserved1;
	avb_u16 flags;
	avb_s64 correction_field;
	avb_u32 reserved2;
	struct ptp_port_identity source_port_id;
	avb_u16 sequence_id;
	avb_u8 control;
	avb_s8 log_msg_interval;
};

#endif /* _GENAVB_PUBLIC_PTP_H_ */
