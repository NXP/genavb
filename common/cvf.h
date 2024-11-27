/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		cvf.h
 @brief   	CVF protocol common definitions
 @details	Protocol and Header definitions for all CVF applications
*/

#ifndef _PROTO_CVF_H_
#define _PROTO_CVF_H_

#include "common/types.h"
#include "genavb/cvf.h"

struct __attribute__ ((packed)) cvf_mjpeg_hdr {
	u8 type_specific;
	u8 fragment_offset_msb;
	u16 fragment_offset_lsb;
	u8 type;
	u8 Q;
	u8 width;
	u8 height;
};

struct __attribute__ ((packed)) cvf_h264_hdr {
	u32 h264_timestamp;
};

struct __attribute__ ((packed)) cvf_h264_nalu_header {
#ifdef __BIG_ENDIAN__
	u8 f:1;
	u8 nri:2;
	u8 type:5;
#else
	u8 type:5;
	u8 nri:2;
	u8 f:1;
#endif
};

#define cvf_h264_nalu_fu_indicator	cvf_h264_nalu_header

struct __attribute__ ((packed)) cvf_h264_nalu_fu_header {
#ifdef __BIG_ENDIAN__
	u8 s:1;
	u8 e:1;
	u8 r:1;
	u8 type:5;
#else
	u8 type:5;
	u8 r:1;
	u8 e:1;
	u8 s:1;
#endif
};

#define FU_HEADER_SIZE	(sizeof(struct cvf_h264_nalu_fu_header) + sizeof(struct cvf_h264_nalu_fu_indicator))

/* NALU types as defined in the RFC6184 section 5.4, table 3
 * NAL Unit types is also specified in T-REC-H.264 Section 7.4.1 Table 7.1 */
enum cvf_h264_nalu_type {
	CVF_H264_NALU_TYPE_RESERVED0 = 0,
	CVF_H264_NALU_TYPE_NAL_UNIT1,
	CVF_H264_NALU_TYPE_NAL_UNIT2,
	CVF_H264_NALU_TYPE_NAL_UNIT3,
	CVF_H264_NALU_TYPE_NAL_UNIT4,
	CVF_H264_NALU_TYPE_NAL_UNIT5,
	CVF_H264_NALU_TYPE_NAL_UNIT6,
	CVF_H264_NALU_TYPE_NAL_UNIT7,
	CVF_H264_NALU_TYPE_NAL_UNIT8,
	CVF_H264_NALU_TYPE_NAL_UNIT9,
	CVF_H264_NALU_TYPE_NAL_UNIT10,
	CVF_H264_NALU_TYPE_NAL_UNIT11,
	CVF_H264_NALU_TYPE_NAL_UNIT12,
	CVF_H264_NALU_TYPE_NAL_UNIT13,
	CVF_H264_NALU_TYPE_NAL_UNIT14,
	CVF_H264_NALU_TYPE_NAL_UNIT15,
	CVF_H264_NALU_TYPE_NAL_UNIT16,
	CVF_H264_NALU_TYPE_RESERVED17,
	CVF_H264_NALU_TYPE_RESERVED18,
	CVF_H264_NALU_TYPE_NAL_UNIT19,
	CVF_H264_NALU_TYPE_NAL_UNIT20,
	CVF_H264_NALU_TYPE_NAL_UNIT21,
	CVF_H264_NALU_TYPE_RESERVED22,
	CVF_H264_NALU_TYPE_RESERVED23,
	CVF_H264_NALU_TYPE_STAP_A,
	CVF_H264_NALU_TYPE_STAP_B,
	CVF_H264_NALU_TYPE_MTAP16,
	CVF_H264_NALU_TYPE_MTAP24,
	CVF_H264_NALU_TYPE_FU_A,
	CVF_H264_NALU_TYPE_FU_B,
	CVF_H264_NALU_TYPE_RESERVED30,
	CVF_H264_NALU_TYPE_RESERVED31
};

#define CVF_H264_STAP_A_NALU_SIZE_HDR_SIZE		2

#endif /* PROTO_CVF_H */
