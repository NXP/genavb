/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file cvf.h
 \brief GenAVB public API
 \details CVF (AVTP Compressed Video Format) header definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_CVF_H_
#define _GENAVB_PUBLIC_CVF_H_

/**
* \defgroup cvf			CVF
*  Compressed Video format as defined in IEEE 1722-2016, section 8
* \ingroup avtp
*/

/** Compressed Video Format
 *  IEEE 1722-2016, Section 8, Table 19
 * \ingroup cvf
 *
 */
enum cvf_format {
	CVF_FORMAT_RFC = 0x2	/**< RFC payload Type */
};

/** Compressed Video Format
 *  IEEE 1722-2016, Section 8, Table 20
 * \ingroup cvf
 *
 */
enum cvf_format_subtype {
	CVF_FORMAT_SUBTYPE_MJPEG = 0x0,		/**< MJPEG Format (RFC 2435) */
	CVF_FORMAT_SUBTYPE_H264 = 0x1,		/**< H.264 Format (RFC 6184) */
	CVF_FORMAT_SUBTYPE_JPEG2000 = 0x2	/**< JPEG 2000 Video (RFC 5371) */
};

/**
* \defgroup cvf_mjpeg			MJPEG
*  Compressed Video format as defined in IEEE 1722-2016, section 8.4
* \ingroup cvf
*/

/** MJPEG Scan Type
 * \ingroup cvf_mjpeg
 *
 */
enum cvf_mjpeg_p {
	CVF_MJPEG_P_INTERLACE = 0x0,	/**< Interlace Scanning */
	CVF_MJPEG_P_PROGRESSIVE = 0x1	/**< Progressive Scanning */
};

/** MJPEG Type
 * \ingroup cvf_mjpeg
 *
 */
enum cvf_mjpeg_type {
	CVF_MJPEG_TYPE_YUV422 = 0x0,	/**< YUV 4:2:2 */
	CVF_MJPEG_TYPE_YUV420 = 0x1,	/**< YUV 4:2:0 */
	CVF_MJPEG_TYPE_RESTART_MARKER = 0x40
};

/**
 * \ingroup cvf_mjpeg
 * @{
 */
#define CVF_MJPEG_TYPE_SPEC_PROGRESSIVE	0x0
#define CVF_MJPEG_TYPE_SPEC_ODD		0x1
#define CVF_MJPEG_TYPE_SPEC_EVEN	0x2
#define CVF_MJPEG_TYPE_SPEC_SINGLE	0x3

#define SCAN_TYPE_2_STR(sc)		(sc == 0? "progressive":"interleave")
/** @} */

/**
 * \ingroup cvf_h264
 */

enum spec_version {
	CVF_H264_IEEE_1722_2016 = 0x0,	/**< Default spec version as defined in IEEE 1722-2016*/
	CVF_H264_IEEE_1722_2013 = 0x1,	/**< Custom spec version to support Simple Video Format 1722-2013 */
	CVF_H264_CUSTOM = 0x2
};

/**
 * \ingroup cvf
 */
struct __attribute__ ((packed)) avtp_cvf_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

	avb_u8 format;
	avb_u8 format_subtype;
	avb_u16 reserved_1;

	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 rsv_1:3;
	avb_u8 M:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 M:1;
	avb_u8 rsv_1:3;
#endif

	avb_u8 reserved_2;
};

/**
 * \ingroup cvf_h264
 */
struct __attribute__ ((packed)) avtp_cvf_h264_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

	avb_u8 format;
	avb_u8 format_subtype;
	avb_u16 reserved_1;

	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 rsv_1:2;
	avb_u8 ptv:1;
	avb_u8 M:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 M:1;
	avb_u8 ptv:1;
	avb_u8 rsv_1:2;
#endif

	avb_u8 reserved_2;
};

/**
 * \ingroup cvf_mjpeg
 */
struct __attribute__ ((packed)) avtp_cvf_mjpeg_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

	avb_u8 format;
	avb_u8 format_subtype;
	avb_u16 reserved_1;

	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 rsv_1:3;
	avb_u8 M:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 M:1;
	avb_u8 rsv_1:3;
#endif

	avb_u8 reserved_2;
};



#endif /* GENAVB_PUBLIC_CVF_H */
