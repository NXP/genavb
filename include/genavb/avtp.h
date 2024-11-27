/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file avtp.h
 \brief GenAVB/TSN public API
 \details AVTP subtype definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2019, 2021-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_AVTP_H_
#define _GENAVB_PUBLIC_AVTP_H_

#include "types.h"
#include "config.h"
#include "sr_class.h"
#include "ether.h"
#include "acf.h"

/**
* \defgroup avtp	AVTP
* Audio Video transport protocol as defined in IEEE 1722-2011 and IEEE 1722-2016, section 4
* \ingroup protocol
*/

/** AVTP common header
 * \ingroup avtp
 */
struct __attribute__ ((packed)) avtp_common_hdr {
	avb_u8 subtype;

#ifdef __BIG_ENDIAN__
	avb_u8 h:1;
	avb_u8 version:3;
#else
	avb_u8 padding:4;
	avb_u8 version:3;
	avb_u8 h:1;
#endif
};


/** AVTP common stream header
 * \ingroup avtp
 */
struct __attribute__ ((packed)) avtp_hdr {
	avb_u8 subtype;

#ifdef __BIG_ENDIAN__
	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 type_specific_data1:4;

	avb_u16 type_specific_data2;

#else
	avb_u8 type_specific_data1:4; /* FIXME this field is broken, since it crosses a byte boundary */
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u16 type_specific_data2; /* FIXME this field is broken, since it crosses a byte boundary */
#endif

	avb_u64 stream_id;
};


/** AVTP common stream header
 * \ingroup avtp
 */
struct __attribute__ ((packed)) avtp_data_hdr {
	avb_u8 subtype;

#ifdef __BIG_ENDIAN__
	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;

#ifdef CFG_AVTP_1722A
	avb_u8 f_s_d:2;
#else
	avb_u8 r:1;
	avb_u8 gv:1;
#endif
	avb_u8 tv:1;

	avb_u8 sequence_num;

#ifdef CFG_AVTP_1722A
	avb_u8 format_specific_data_1:7;
#else
	avb_u8 reserved:7;
#endif
	avb_u8 tu:1;

#else /* !__BIG_ENDIAN__ */

	avb_u8 tv:1;
#ifdef CFG_AVTP_1722A
	avb_u8 f_s_d:2;
#else
	avb_u8 gv:1;
	avb_u8 r:1;
#endif

	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
#ifdef CFG_AVTP_1722A
	avb_u8 format_specific_data_1:7;
#else
	avb_u8 reserved:7;
#endif

#endif /* !__BIG_ENDIAN__ */

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

#ifdef CFG_AVTP_1722A
	avb_u32 format_specific_data_2;
#else
	avb_u32 gateway_info;
#endif

	avb_u16 stream_data_length;

	avb_u16 protocol_specific_header;
};

#ifdef CFG_AVTP_1722A
#define avtp_stream_hdr	avtp_data_hdr

/** AVTP common control header
 * \ingroup avtp
 */
struct __attribute__ ((packed)) avtp_control_hdr {
#ifdef __BIG_ENDIAN__
	avb_u32 subtype:8;

	avb_u32 sv:1;
	avb_u32 version:3;

	avb_u32 format_specific_data:9;

	avb_u32 control_data_length:11;
#else
	avb_u8 subtype;

	avb_u8 format_specific_data_msb:4;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 control_data_length_msb:3;
	avb_u8 format_specific_data_lsb:5;

	avb_u8 control_data_length_lsb;
#endif

	avb_u64 stream_id;
};

/** AVTP alternative header
 * \ingroup avtp
 */
struct __attribute__ ((packed)) avtp_alternative_hdr {
	avb_u8 subtype;

#ifdef __BIG_ENDIAN__
	avb_u8 h:1;
	avb_u8 version:3;
#else
	avb_u8 padding:4;
	avb_u8 version:3;
	avb_u8 h:1;
#endif
};

#endif

#ifdef CFG_AVTP_1722A
#define format_specific_data_3 protocol_specific_header
#endif


/** Compares 32bit gptp timestamps
 * \return	1 if now is before t, 0 otherwise.
 * \param now	32bit gptp timestamp
 * \param t	32bit gptp timestamp
 */
#define avtp_before(now, t)    ((avb_s32)((avb_u32)(t) - (avb_u32)(now)) > 0)

/** Compares 32bit gptp timestamps
 * \return	1 if now is after t, 0 otherwise.
 * \param now	32bit gptp timestamp
 * \param t	32bit gptp timestamp
 */
#define avtp_after(now, t)     avtp_before(t, now)

/** Compares 32bit gptp timestamps
 * \return	1 if now is before or equal to t, 0 otherwise.
 * \param now	32bit gptp timestamp
 * \param t	32bit gptp timestamp
 */
#define avtp_before_eq(now, t)    ((avb_s32)((avb_u32)(t) - (avb_u32)(now)) >= 0)

/** Compares 32bit gptp timestamps
 * \return	1 if now is after or equal to t, 0 otherwise.
 * \param now	32bit gptp timestamp
 * \param t	32bit gptp timestamp
 */
#define avtp_after_eq(now, t)     avtp_before_eq(t, now)


/** Stream direction.
 * \ingroup stream
 */
typedef enum {
	AVTP_DIRECTION_LISTENER = 0,	/**< Listener direction, stream is received by entity. */
	AVTP_DIRECTION_TALKER,			/**< Talker direction, stream is sent by entity. */
} avtp_direction_t;


/**
 * \ingroup avtp
 * @{
 */

#define AVTP_VERSION_0		0	/**< AVTP Version 0 */

/* IEEE P1722-rev1/D13 Apr 2015 Table 5.4 */
/* IEEE 1722.1-2013 */
/* Note: for 1722-2011, cd and subtype were merged into subtype as required by 1722a drafts to simplify the code */
#define AVTP_SUBTYPE_61883_IIDC		0x00	/**< IEC 61883/IIDC Stream Format */
#define AVTP_SUBTYPE_MMA_STREAM		0x01

#ifdef CFG_AVTP_1722A
#define AVTP_SUBTYPE_AAF			0x02	/**< AVTP Audio Stream Format */
#define AVTP_SUBTYPE_CVF			0x03	/**< Compressed Video Stream Format */
#define AVTP_SUBTYPE_CRF			0x04	/**< Clock Reference Stream Format */
#define AVTP_SUBTYPE_TSCF			0x05	/**< AVTP Time Synchronous Control Format */
#define AVTP_SUBTYPE_SVF			0x06
#define AVTP_SUBTYPE_RVF			0x07

#define AVTP_SUBTYPE_AEF_CONTINUOUS	0x6e
#define AVTP_SUBTYPE_VSF_STREAM		0x6f
#endif

#define AVTP_SUBTYPE_EF_STREAM		0x7f

#ifdef CFG_AVTP_1722A
#define AVTP_SUBTYPE_MMA_CONTROL	0x81
#define AVTP_SUBTYPE_NTSCF			0x82	/**< AVTP Non Time Synchronous Control Format */

#define AVTP_SUBTYPE_ESCF			0xec
#define AVTP_SUBTYPE_EECF			0xed
#define AVTP_SUBTYPE_AEF_DISCRETE	0xee
#endif

#define AVTP_SUBTYPE_ADP			0xfa
#define AVTP_SUBTYPE_AECP			0xfb
#define AVTP_SUBTYPE_ACMP			0xfc

#define AVTP_SUBTYPE_MAAP			0xfe
#define AVTP_SUBTYPE_EF_CONTROL		0xff

#define AVTP_SUBTYPE_AVDECC		0x100 /* Non standard value to match ADP + AECP + ACMP subtypes */

/** @} */


/**
 * \ingroup stream
 * @{
 */
/* Receive events */
#define AVTP_MEDIA_CLOCK_RESTART	(1 << 0)	/**< Media clock restart event, based on AVTP stream header mr bit */
#define AVTP_PACKET_LOST		(1 << 1)	/**< AVTP packet loss event, base on AVTP stream format sequence number */
#define AVTP_TIMESTAMP_UNCERTAIN	(1 << 2)	/**< AVTP timestamp uncertain event, based on AVTP stream header tu bit */
#define AVTP_END_OF_FRAME		(1 << 14)	/**< End of frame event, based on AVTP CVF M bit */
#define AVTP_TIMESTAMP_INVALID		(1 << 15)	/**< AVTP timestamp invalid event, based on AVTP stream header tv bit */

/* Send events */
#define AVTP_SYNC			(1 << 0)	/**< Synchronization event */
#define AVTP_FLUSH			(1 << 1)	/**< Flush event */
#define AVTP_FRAME_END			(1 << 2)	/**< End of frame event */

/** @} */

#define AVTP_DATA_MTU	(ETHER_MTU - sizeof(struct avtp_data_hdr))

static inline int is_avtp_stream(unsigned int subtype)
{
	/* Small optimization for most common stream subtypes */
	if (subtype < AVTP_SUBTYPE_CRF)
		return 1;

	switch (subtype) {
	case AVTP_SUBTYPE_TSCF:
	case AVTP_SUBTYPE_SVF:
	case AVTP_SUBTYPE_RVF:
	case AVTP_SUBTYPE_VSF_STREAM:
	case AVTP_SUBTYPE_EF_STREAM:
		return 1;

	default:
		return 0;
	}

	return 0;
}

static inline int is_avtp_alternative(unsigned int subtype)
{
	switch (subtype) {
	case AVTP_SUBTYPE_CRF:
	case AVTP_SUBTYPE_AEF_CONTINUOUS:
	case AVTP_SUBTYPE_NTSCF:
	case AVTP_SUBTYPE_ESCF:
	case AVTP_SUBTYPE_EECF:
	case AVTP_SUBTYPE_AEF_DISCRETE:
		return 1;

	default:
		return 0;
	}

	return 0;
}

static inline int is_avtp_avdecc(unsigned int subtype)
{
	switch (subtype) {
	case AVTP_SUBTYPE_ADP:
	case AVTP_SUBTYPE_AECP:
	case AVTP_SUBTYPE_ACMP:
	case AVTP_SUBTYPE_MAAP:
	case AVTP_SUBTYPE_AVDECC:
		return 1;

	default:
		return 0;
	}

	return 0;
}

#define PST_MAX	1000 /* Max gPTP Skew Time, 1us for 7 or fewer hops 802.1AS-2011, Annex B3 */

/* AVTP timestamp validity */
/* 1722, Equation(1) */
/* tt > avtp_ts - max_transit_time - max_timing_uncertainty - tsamples */
/* avtp_ts - max_transit_time - tsamples > tt */

/* ingress time reference plane, presentation time reference plane, mac transmit and receive time */
/* tx_ts > tt, mac transmit is after ingress reference plane */
/* rx_ts > tx_ts - PST_MAX => rx_ts + PST_MAX > tt, mac receive time is after transmit */
/* rx_ts < tt + max_transit_time + PST_MAX <=> tt > rx_ts - max_transit_time - PST_MAX, receive time is before presentation reference plane */

/* Putting the two together */
/* (a) rx_ts + PST_MAX > avtp_ts - max_transit_time - max_timing_uncertainty - tsamples */
/* (b) avtp_ts - max_transit_time - tsamples > rx_ts - max_transit_time - PST_MAX */
/* (a) rx_ts + max_transit_time + max_timing_uncertainty + tsamples + PST_MAX > avtp_ts */
/* (b) avtp_ts > rx_ts + tsamples - PST_MAX */
static inline int is_avtp_ts_late(avb_u32 rx_ts, avb_u32 avtp_ts, unsigned int tsamples)
{
	return avtp_before(avtp_ts, rx_ts + tsamples - PST_MAX);
}

static inline int is_avtp_ts_early(avb_u32 rx_ts, avb_u32 avtp_ts, unsigned int max_transit_time, unsigned int max_timing_uncertainty, unsigned int tsamples)
{
	return avtp_after(avtp_ts, rx_ts + max_transit_time + max_timing_uncertainty + tsamples + PST_MAX);
}

static inline int is_avtp_ts_valid(avb_u32 rx_ts, avb_u32 avtp_ts, unsigned int max_transit_time, unsigned int max_timing_uncertainty, unsigned int tsamples)
{
	if (!is_avtp_ts_late(rx_ts, avtp_ts, tsamples) &&
	    !is_avtp_ts_early(rx_ts, avtp_ts, max_transit_time, max_timing_uncertainty, tsamples))
		return 1;

	return 0;
}

static inline unsigned int avtp_mtu(unsigned int subtype)
{
	switch (subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
	case AVTP_SUBTYPE_CVF:
	case AVTP_SUBTYPE_TSCF:
		return AVTP_DATA_MTU;

	case AVTP_SUBTYPE_NTSCF:
		return ETHER_MTU - sizeof(struct avtp_ntscf_hdr);

	default:
		return 0;
	}

	return 0;
}

#endif /* _GENAVB_PUBLIC_AVTP_H_ */
