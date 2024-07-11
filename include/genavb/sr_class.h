/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  \file		sr_class.h
  \brief	SR classes definitions
  \details

  \copyright Copyright 2015 Freescale Semiconductor, Inc.
  \copyright Copyright 2016, 2018-2023 NXP
*/
#ifndef _GENAVB_PUBLIC_SR_CLASS_H_
#define _GENAVB_PUBLIC_SR_CLASS_H_

#include "types.h"

/** Stream classes.
 * \ingroup stream
 */

/* IEEE-1722-2011 Table 5.3, 5.4 */
#define SR_CLASS_A_MAX_TIMING_UNCERTAINTY	125000
#define SR_CLASS_A_MAX_TRANSIT_TIME		2000000
#define SR_CLASS_A_INTERVAL_P			125000
#define SR_CLASS_A_INTERVAL_Q			1
#define SR_CLASS_A_MAX_INTERVAL_FRAMES	2

#define SR_CLASS_B_MAX_TIMING_UNCERTAINTY	250000
#define SR_CLASS_B_MAX_TRANSIT_TIME		10000000 /* 50000000, only if wifi is supported */
#define SR_CLASS_B_INTERVAL_P			250000
#define SR_CLASS_B_INTERVAL_Q			1
#define SR_CLASS_B_MAX_INTERVAL_FRAMES	4

/* Automotive Ethernet AVB Functional and Interoperability Specification 1.4, Table 15, 18 & 20 */
#define SR_CLASS_C_MAX_TIMING_UNCERTAINTY	500000
#define SR_CLASS_C_MAX_TRANSIT_TIME		15000000
#define SR_CLASS_C_INTERVAL_P			4000000
#define SR_CLASS_C_INTERVAL_Q			3
#define SR_CLASS_C_MAX_INTERVAL_FRAMES	8

#define SR_CLASS_D_MAX_TIMING_UNCERTAINTY	500000
#define SR_CLASS_D_MAX_TRANSIT_TIME		15000000
#define SR_CLASS_D_INTERVAL_P			640000000
#define SR_CLASS_D_INTERVAL_Q			441
#define SR_CLASS_D_MAX_INTERVAL_FRAMES	8

#define SR_CLASS_E_MAX_TIMING_UNCERTAINTY	500000
#define SR_CLASS_E_MAX_TRANSIT_TIME		15000000
#define SR_CLASS_E_INTERVAL_P			1000000
#define SR_CLASS_E_INTERVAL_Q			1
#define SR_CLASS_E_MAX_INTERVAL_FRAMES	8

/* 802.1Q-2011 Table 34-1, 34-2 */
#define SR_CLASS_HIGH_PCP			3	/* FIXME This could be changed at run time through SRP domain discovery */
#define SR_CLASS_LOW_PCP			2	/* FIXME This could be changed at run time through SRP domain discovery */

/* 802.1Q-2011 Table 35-7 */
/* Automotive Ethernet AVB Functional and Interoperability Specification 1.4, Table 16 & 17 */
#define SR_CLASS_HIGH_ID			6
#define SR_CLASS_LOW_ID				5

#define CFG_SR_CLASS_MAX	2

/**
 * \ingroup stream
 * SRP Stream Class
 */
typedef enum {
	SR_CLASS_MIN = 0,
	SR_CLASS_A = SR_CLASS_MIN,	/**< SR class A, 802.1Q-2011, 3.175 */
	SR_CLASS_B,			/**< SR class B, 802.1Q-2011, 3.175 */
	SR_CLASS_C,
	SR_CLASS_D,
	SR_CLASS_E,
	SR_CLASS_MAX,
	SR_CLASS_NONE = SR_CLASS_MAX	/**< No stream reservation */
} sr_class_t;

typedef enum {
	SR_PRIO_MIN = 0,
	SR_PRIO_HIGH = SR_PRIO_MIN,
	SR_PRIO_LOW,
	SR_PRIO_MAX,
	SR_PRIO_NONE = SR_PRIO_MAX
} sr_prio_t;

sr_class_t sr_class_high(void);

sr_class_t sr_class_low(void);

unsigned int sr_class_enabled(sr_class_t sr_class);

unsigned int sr_class_prio(sr_class_t sr_class);

unsigned int sr_class_max_timing_uncertainty(sr_class_t sr_class);

unsigned int sr_class_max_transit_time(sr_class_t sr_class);

unsigned int sr_class_max_interval_frames(sr_class_t sr_class);

unsigned int sr_class_interval_p(sr_class_t sr_class);

unsigned int sr_class_interval_q(sr_class_t sr_class);

sr_class_t sr_prio_class(sr_prio_t prio);

unsigned int sr_prio_pcp(sr_prio_t prio);

unsigned int sr_prio_id(sr_prio_t prio);

sr_class_t sr_pcp_class(unsigned int pcp);

unsigned int sr_class_pcp(sr_class_t sr_class);

unsigned int sr_class_id(sr_class_t sr_class);

int sr_class_config(uint8_t *sr_class);

sr_class_t str_to_sr_class(const char *s);

#endif /* _GENAVB_PUBLIC_SR_CLASS_H_ */
