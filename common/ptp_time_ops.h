/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2016-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file		ptp_time_ops.h
 @brief	PTP Time operations common definitions
*/

#ifndef _PTP_TIME_OPS_H_
#define _PTP_TIME_OPS_H_

#include "common/types.h"
#include "common/ptp.h"
#include "common/net.h"
#include "common/log.h"
#include "common/timer.h"
#include "os/clock.h"

#define PTP_TS_UNSET_U64_VALUE 0xffffffffffffffffULL

#define POW_2_80	(1.208925819614629e24)
#define POW_2_79	(6.044629098073146e23)
#define POW_2_64	(18446744073709551616.0)
#define POW_2_41	(2199023255552.0)
#define POW_2_32	(4294967296.0)
#define POW_2_16	(65536.0)

#define PTP_TIMESTAMP_MAX_SECONDS_MSB	4
static inline u64 pdu_ptp_timestamp_to_u64(struct ptp_timestamp ptp_ts)
{
	u64 nanoseconds = (u64)ntohl(ptp_ts.nanoseconds) + ((u64)ntohl(ptp_ts.seconds_lsb) + ((u64)ntohs(ptp_ts.seconds_msb) << 32))*NSECS_PER_SEC;
	if (ntohs(ptp_ts.seconds_msb) >= PTP_TIMESTAMP_MAX_SECONDS_MSB)
		os_log(LOG_ERR, "Likely overflow in PTP timestamp structure (seconds_msb>=4): seconds_msb = %u seconds_lsb = %u nanoseconds = %u\n",
				ntohs(ptp_ts.seconds_msb), ntohl(ptp_ts.seconds_lsb), ntohl(ptp_ts.nanoseconds));

	return nanoseconds;
}

static inline void ntoh_ptp_timestamp(struct ptp_timestamp *h_ptp_ts, const struct ptp_timestamp *n_ptp_ts)
{
	h_ptp_ts->seconds_msb = ntohs(n_ptp_ts->seconds_msb);
	h_ptp_ts->seconds_lsb = ntohl(n_ptp_ts->seconds_lsb);
	h_ptp_ts->nanoseconds = ntohl(n_ptp_ts->nanoseconds);
}

static inline void ntoh_scaled_ns(struct ptp_scaled_ns *h_scaled_ns, const struct ptp_scaled_ns *n_scaled_ns)
{
	h_scaled_ns->u.s.nanoseconds_msb = ntohs(n_scaled_ns->u.s.nanoseconds_msb);
	h_scaled_ns->u.s.nanoseconds = ntohll(n_scaled_ns->u.s.nanoseconds);
	h_scaled_ns->u.s.fractional_nanoseconds = ntohs(n_scaled_ns->u.s.fractional_nanoseconds);
}

static inline void hton_scaled_ns(struct ptp_scaled_ns *n_scaled_ns, const struct ptp_scaled_ns *h_scaled_ns)
{
	n_scaled_ns->u.s.nanoseconds_msb = htons(h_scaled_ns->u.s.nanoseconds_msb);
	n_scaled_ns->u.s.nanoseconds = htonll(h_scaled_ns->u.s.nanoseconds);
	n_scaled_ns->u.s.fractional_nanoseconds = htons(h_scaled_ns->u.s.fractional_nanoseconds);
}

static inline void scaled_ns_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_scaled_ns *scaled_ns)
{
	/* This is a cast from a 96bit signed integer to a 96bit unsigned integer, all bits (including sign bit) remain in the same place */
	u_scaled_ns->u.s.nanoseconds_msb = scaled_ns->u.s.nanoseconds_msb;
	u_scaled_ns->u.s.nanoseconds = scaled_ns->u.s.nanoseconds;
	u_scaled_ns->u.s.fractional_nanoseconds = scaled_ns->u.s.fractional_nanoseconds;
}

static inline void u_scaled_ns_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, const struct ptp_u_scaled_ns *u_scaled_ns)
{
	/* This is a cast from a 96bit unsigned integer to a 96bit signed integer, all bits (including sign bit) remain in the same place */
	scaled_ns->u.s.nanoseconds_msb = u_scaled_ns->u.s.nanoseconds_msb;
	scaled_ns->u.s.nanoseconds = u_scaled_ns->u.s.nanoseconds;
	scaled_ns->u.s.fractional_nanoseconds = u_scaled_ns->u.s.fractional_nanoseconds;
}

/* Two's complement of a 96bit unsigned integer */
static inline void minus_u_scaled_ns(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a)
{
	/* Two's complement */

	/* One's complement */
	result->u.s.nanoseconds_msb = ~a->u.s.nanoseconds_msb;
	result->u.s.nanoseconds = ~a->u.s.nanoseconds;
	result->u.s.fractional_nanoseconds = ~a->u.s.fractional_nanoseconds;

	/* +1 */
	result->u.s.fractional_nanoseconds++;
	if (result->u.s.fractional_nanoseconds < 1) {
		/* Carry */
		result->u.s.nanoseconds++;

		if (result->u.s.nanoseconds < 1)
			/* Carry */
			result->u.s.nanoseconds_msb++;
	}
}

static inline void u64_to_pdu_ptp_timestamp(struct ptp_timestamp *ts, u64 nanoseconds)
{
	u64 seconds;

	seconds = nanoseconds / NSECS_PER_SEC;
	ts->seconds_msb = htons(seconds >> 32);
	ts->seconds_lsb = htonl(seconds & 0xffffffff);
	ts->nanoseconds = htonl(nanoseconds % NSECS_PER_SEC);
}

static inline void u64_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, u64 u64_ts_ns)
{
	u_scaled_ns->u.s.nanoseconds_msb = 0;	// FIXME ignored, see u_scaled_ns structure definition
	u_scaled_ns->u.s.nanoseconds = u64_ts_ns;
	u_scaled_ns->u.s.fractional_nanoseconds = 0;
}

/* This function truncates fractional nanoseconds */
static inline void ptp_extended_timestamp_to_ptp_timestamp(struct ptp_timestamp *ptp_ts, const struct ptp_extended_timestamp *ptp_ext_ts)
{
	ptp_ts->seconds_msb = ptp_ext_ts->seconds_msb;
	ptp_ts->seconds_lsb = ptp_ext_ts->seconds_lsb;
	ptp_ts->nanoseconds = ptp_ext_ts->fractional_nanoseconds_msb;
}

static inline void ptp_timestamp_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, const struct ptp_timestamp *ptp_ts)
{
	ptp_ext_ts->seconds_msb = ptp_ts->seconds_msb;
	ptp_ext_ts->seconds_lsb = ptp_ts->seconds_lsb;
	ptp_ext_ts->fractional_nanoseconds_msb = ptp_ts->nanoseconds;
	ptp_ext_ts->fractional_nanoseconds_lsb = 0;
}

static inline void ptp_extended_timestamp_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_extended_timestamp *ptp_ext_ts)
{
	u64 tmp, tmp1;

	tmp = (u64)ptp_ext_ts->seconds_msb * NSECS_PER_SEC; /* nanoseconds >> 32 units */

	u_scaled_ns->u.s.nanoseconds_msb = tmp >> 32;

	tmp1 = ptp_ext_ts->fractional_nanoseconds_msb + (u64)ptp_ext_ts->seconds_lsb * NSECS_PER_SEC;
	if (tmp1 < ptp_ext_ts->fractional_nanoseconds_msb)
		u_scaled_ns->u.s.nanoseconds_msb++;

	u_scaled_ns->u.s.nanoseconds = tmp1 + ((tmp & 0xffffffff) << 32);
	if (u_scaled_ns->u.s.nanoseconds < tmp1)
		u_scaled_ns->u.s.nanoseconds_msb++;

	u_scaled_ns->u.s.fractional_nanoseconds = ptp_ext_ts->fractional_nanoseconds_lsb;
}

static inline void u_scaled_ns_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, const struct ptp_u_scaled_ns *u_scaled_ns)
{
	u16 seconds_msb;
	u64 seconds;
	u32 nanoseconds;

	seconds_msb = ((u64)u_scaled_ns->u.s.nanoseconds_msb << 32) / NSECS_PER_SEC;	/* seconds >> 32 units, remainder 0*/

	seconds = u_scaled_ns->u.s.nanoseconds / NSECS_PER_SEC;
	nanoseconds = u_scaled_ns->u.s.nanoseconds - seconds * NSECS_PER_SEC;

	ptp_ext_ts->seconds_msb = (seconds >> 32) + seconds_msb;
	ptp_ext_ts->seconds_lsb = seconds & 0xffffffff;
	ptp_ext_ts->fractional_nanoseconds_msb = nanoseconds;
	ptp_ext_ts->fractional_nanoseconds_lsb = u_scaled_ns->u.s.fractional_nanoseconds;
}

static inline void u64_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, u64 u64_ts_ns)
{
	struct ptp_u_scaled_ns u_scaled_ns;

	u64_to_u_scaled_ns(&u_scaled_ns, u64_ts_ns);
	u_scaled_ns_to_ptp_extended_timestamp(ptp_ext_ts, &u_scaled_ns);
}

static inline void ptp_timestamp_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, const struct ptp_timestamp *ptp_ts)
{
	struct ptp_extended_timestamp ptp_ext_ts;

	ptp_timestamp_to_ptp_extended_timestamp(&ptp_ext_ts, ptp_ts);

	ptp_extended_timestamp_to_u_scaled_ns(u_scaled_ns, &ptp_ext_ts);
}


static inline void hton_ptp_timestamp(struct ptp_timestamp *pdu_ptp_ts, const struct ptp_timestamp *ptp_ts)
{
	pdu_ptp_ts->seconds_msb = htons(ptp_ts->seconds_msb);
	pdu_ptp_ts->seconds_lsb = htonl(ptp_ts->seconds_lsb);
	pdu_ptp_ts->nanoseconds = htonl(ptp_ts->nanoseconds);
}

static inline void ptp_double_to_u_scaled_ns(struct ptp_u_scaled_ns *u_scaled_ns, ptp_double double_ts_ns)
{
	ptp_double nanoseconds;

	u_scaled_ns->u.s.nanoseconds_msb = 0;
	u_scaled_ns->u.s.nanoseconds = 0;
	u_scaled_ns->u.s.fractional_nanoseconds = 0;

	if (double_ts_ns < 0) {
		os_log(LOG_ERR, "Trying to convert a negative value (%10.3f) to u_scaled_ns type, returning 0.\n", double_ts_ns);
		return;
	}

	if (double_ts_ns > POW_2_80)
		os_log(LOG_ERR, "Conversion overflow %f\n", double_ts_ns);

	u_scaled_ns->u.s.nanoseconds_msb = double_ts_ns / POW_2_64;
	nanoseconds = double_ts_ns - u_scaled_ns->u.s.nanoseconds_msb * POW_2_64;
	u_scaled_ns->u.s.nanoseconds = nanoseconds;
	u_scaled_ns->u.s.fractional_nanoseconds = (nanoseconds - u_scaled_ns->u.s.nanoseconds)*(1 << 16);
}

static inline void ptp_double_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, ptp_double double_ts_ns)
{
	struct ptp_u_scaled_ns tmp;
	unsigned int negative = 0;

	if (double_ts_ns > POW_2_79)
		os_log(LOG_ERR, "Conversion overflow %f\n", double_ts_ns);

	if (double_ts_ns < 0.0) {
		negative = 1;
		double_ts_ns = -double_ts_ns;
	}

	ptp_double_to_u_scaled_ns(&tmp, double_ts_ns);

	if (negative)
		minus_u_scaled_ns(&tmp, &tmp);

	u_scaled_ns_to_scaled_ns(scaled_ns, &tmp);
}


static inline void ptp_double_to_ptp_extended_timestamp(struct ptp_extended_timestamp *ptp_ext_ts, ptp_double double_ts_ns)
{
	struct ptp_u_scaled_ns u_scaled_ns;

	ptp_double_to_u_scaled_ns(&u_scaled_ns, double_ts_ns);
	u_scaled_ns_to_ptp_extended_timestamp(ptp_ext_ts, &u_scaled_ns);
}


static inline void pdu_correction_field_to_scaled_ns(struct ptp_scaled_ns *scaled_ns, s64 correction_field)
{
	struct ptp_u_scaled_ns u_scaled_ns;
	s64 correction_field_host = ntohll(correction_field); /* the sign bit position it's correct after endianess conversion*/
	unsigned int negative = 0;

	if (correction_field_host < 0) {
		negative = 1;
		correction_field_host = -correction_field_host;
	}

	u_scaled_ns.u.s.nanoseconds_msb = 0;
	u_scaled_ns.u.s.nanoseconds = (correction_field_host >> 16) & 0xffffffffffffffff;
	u_scaled_ns.u.s.fractional_nanoseconds = correction_field_host & 0xffff;

	if (negative)
		minus_u_scaled_ns(&u_scaled_ns, &u_scaled_ns);

	u_scaled_ns_to_scaled_ns(scaled_ns, &u_scaled_ns);
}

static inline void scaled_ns_to_pdu_correction_field(s64 *correction_field, struct ptp_scaled_ns *scaled_ns)
{
	struct ptp_u_scaled_ns u_scaled_ns;
	s64 correction_field_host;
	unsigned int negative = 0;

	scaled_ns_to_u_scaled_ns(&u_scaled_ns, scaled_ns);

	if (scaled_ns->u.s.nanoseconds_msb & 0x8000) { /* ptp_scaled_ns struct has unsigned fields */
		negative = 1;
		minus_u_scaled_ns(&u_scaled_ns, &u_scaled_ns);
	}

	correction_field_host = u_scaled_ns.u.s.fractional_nanoseconds + (u_scaled_ns.u.s.nanoseconds << 16);

	if (negative)
		correction_field_host = -correction_field_host;

	*correction_field = htonll(correction_field_host);
}


/* Unsigned integer overflow: drop higher bits */
/* Unsigned integer negation: -a = ~a + 1 (two's complement) */
/* Unsigned integer subtraction: a - b = a + (-b) */

/* Adds two 96bit unsigned integers with correct overflow handling */
static inline void u_scaled_ns_add(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b)
{
	struct ptp_u_scaled_ns tmp;

	tmp.u.s.fractional_nanoseconds = a->u.s.fractional_nanoseconds + b->u.s.fractional_nanoseconds;
	tmp.u.s.nanoseconds = a->u.s.nanoseconds + b->u.s.nanoseconds;
	tmp.u.s.nanoseconds_msb = a->u.s.nanoseconds_msb + b->u.s.nanoseconds_msb;

	if (tmp.u.s.nanoseconds < a->u.s.nanoseconds)
		tmp.u.s.nanoseconds_msb++;

	if (tmp.u.s.fractional_nanoseconds < a->u.s.fractional_nanoseconds) {
		tmp.u.s.nanoseconds++;
		if (tmp.u.s.nanoseconds < 1)
			tmp.u.s.nanoseconds_msb++;
	}

	*result = tmp;
}

/* Subtracts two 96bit unsigned integers with correct support for "negative" values */
static inline void u_scaled_ns_sub(struct ptp_u_scaled_ns *result, const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b)
{
	struct ptp_u_scaled_ns tmp;

	minus_u_scaled_ns(&tmp, b);
	u_scaled_ns_add(result, a, &tmp);
}

/* This function does rounding as well */
static inline void u_scaled_ns_to_u64(u64 *u64_ts_ns, const struct ptp_u_scaled_ns *u_scaled_ns)
{
	if (u_scaled_ns->u.s.nanoseconds_msb)
		os_log(LOG_ERR, "Conversion overflow %u.%"PRIu64".%u\n",
		       u_scaled_ns->u.s.nanoseconds_msb, u_scaled_ns->u.s.nanoseconds, u_scaled_ns->u.s.fractional_nanoseconds);

	*u64_ts_ns = u_scaled_ns->u.s.nanoseconds;
	if (u_scaled_ns->u.s.fractional_nanoseconds >= 32768)
		*u64_ts_ns += 1;
}

/* This function may loose precision */
static inline void u_scaled_ns_to_ptp_double( ptp_double *double_ts_ns, const struct ptp_u_scaled_ns *u_scaled_ns)
{
	*double_ts_ns = ((ptp_double)u_scaled_ns->u.s.nanoseconds_msb * POW_2_64) + u_scaled_ns->u.s.nanoseconds + ((ptp_double)u_scaled_ns->u.s.fractional_nanoseconds / POW_2_16);

	if (u_scaled_ns->u.s.nanoseconds_msb || (u_scaled_ns->u.s.nanoseconds > (1ULL << 53)))
		os_log(LOG_ERR, "Possible precision truncation %u.%"PRIu64".%u != %f\n",
		       u_scaled_ns->u.s.nanoseconds_msb, u_scaled_ns->u.s.nanoseconds, u_scaled_ns->u.s.fractional_nanoseconds, *double_ts_ns);
}

/* This function compares two ptp_scaled_ns values a and b. It returns an interger
less than, equal to, or greater than 0 if a is found respectively, to be less, to match, or be greater
than b */
static inline int u_scaled_ns_cmp(const struct ptp_u_scaled_ns *a, const struct ptp_u_scaled_ns *b)
{
	if (a->u.s.nanoseconds_msb < b->u.s.nanoseconds_msb)
		return -1;
	if (a->u.s.nanoseconds_msb > b->u.s.nanoseconds_msb)
		return 1;

	if (a->u.s.nanoseconds < b->u.s.nanoseconds)
		return -1;
	if (a->u.s.nanoseconds > b->u.s.nanoseconds)
		return 1;

	if (a->u.s.fractional_nanoseconds < b->u.s.fractional_nanoseconds)
		return -1;
	if (a->u.s.fractional_nanoseconds > b->u.s.fractional_nanoseconds)
		return 1;

	return 0;
}

/* This function may loose precision */
static inline void scaled_ns_to_ptp_double( ptp_double *double_ts_ns, const struct ptp_scaled_ns *scaled_ns)
{
	struct ptp_u_scaled_ns tmp;
	unsigned int negative = 0;

	scaled_ns_to_u_scaled_ns(&tmp, scaled_ns);

	if (scaled_ns->u.s.nanoseconds_msb & 0x8000) {
		negative = 1;
		minus_u_scaled_ns(&tmp, &tmp);
	}

	u_scaled_ns_to_ptp_double(double_ts_ns, &tmp);

	if (negative)
		*double_ts_ns = -*double_ts_ns;
}

/* This function may loose precision */
static inline void ptp_timestamp_to_ptp_double_ns(ptp_double *double_ts_ns, struct ptp_timestamp ptp_ts)
{
	*double_ts_ns = ((ptp_double)ptp_ts.seconds_msb * POW_2_32 * NSECS_PER_SEC) + ptp_ts.seconds_lsb * NSECS_PER_SEC + ptp_ts.nanoseconds;

	if (*double_ts_ns > (1ULL << 53))
		os_log(LOG_ERR, "Possible precision truncation %u.%u.%u != %f\n",
		       ptp_ts.seconds_msb, ptp_ts.seconds_lsb, ptp_ts.nanoseconds, *double_ts_ns);
}

static inline void scaled_ns_add(struct ptp_scaled_ns *result, struct ptp_scaled_ns a, struct ptp_scaled_ns b)
{
	result->u.s.nanoseconds_msb = a.u.s.nanoseconds_msb + b.u.s.nanoseconds_msb;

	result->u.s.fractional_nanoseconds = a.u.s.fractional_nanoseconds + b.u.s.fractional_nanoseconds;

	result->u.s.nanoseconds = a.u.s.nanoseconds + b.u.s.nanoseconds;
	if (result->u.s.fractional_nanoseconds < a.u.s.fractional_nanoseconds)
		result->u.s.nanoseconds += 1;
}


static inline void double_scaled_ns_mul(struct ptp_scaled_ns *result, ptp_double a, struct ptp_scaled_ns b)
{
	ptp_double tmp, carry = 0.0;

	tmp = a * b.u.s.fractional_nanoseconds;
	if (tmp > (1 << 16)) {
		carry = tmp - (1 << 16);
		tmp -= carry;
	} else {
		result->u.s.fractional_nanoseconds = a * b.u.s.fractional_nanoseconds;
	}

	// FIXME ignored, see u_scaled_ns structure definition
	/*tmp = a * b.u.s.nanoseconds + carry;
	if (tmp > (1 << 16)) {

	} else {

	}*/

	result->u.s.nanoseconds_msb = a * b.u.s.nanoseconds_msb;
	result->u.s.nanoseconds = a * b.u.s.nanoseconds;

}

static inline u64 log_to_ns (signed char log_val)
{
	u64 ns;

	/* make sure log interval won't be out of bounds */
	if (log_val > CFG_GPTP_MAX_LOG_INTERVAL) {
		os_log(LOG_DEBUG, "log value %u out of bounds, replaced by %u\n", log_val, CFG_GPTP_MAX_LOG_INTERVAL);
		log_val = CFG_GPTP_MAX_LOG_INTERVAL;
	} else if (log_val < CFG_GPTP_MIN_LOG_INTERVAL) {
		os_log(LOG_DEBUG, "log value %u out of bounds, replaced by %u\n", log_val, CFG_GPTP_MIN_LOG_INTERVAL);
		log_val = CFG_GPTP_MIN_LOG_INTERVAL;
	}

	if (log_val < 0)
		ns = NSECS_PER_SEC / (1 << -log_val);
	else
		ns = (u64)NSECS_PER_SEC * (1 << log_val);

	return ns;
}

static inline u32 log_to_ms (signed char log_val)
{
	return (u32)(log_to_ns(log_val) / NS_PER_MS);
}

#endif /* _PTP_TIME_OPS_H_ */
