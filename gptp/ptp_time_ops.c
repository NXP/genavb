/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP PTP time operations unit test
 @details
*/

#include "common/ptp_time_ops.h"
#include "os/stdlib.h"


static void ptp_double_unit_test(void)
{
	ptp_double d1 = 123456789123456789123.0;
	ptp_double d2 = -d1;
	ptp_double d3 = 123456789123.123;
	struct ptp_u_scaled_ns u;
	struct ptp_scaled_ns s, s1;
	ptp_double d4;

	ptp_double_to_scaled_ns(&s, d1);
	scaled_ns_to_u_scaled_ns(&u, &s);

	u_scaled_ns_to_scaled_ns(&s1, &u);
	scaled_ns_to_ptp_double(&d4, &s1);

	os_log(LOG_INFO, "%f %f %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx\n", d1, d4,
		s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds,
		u.u.s.nanoseconds_msb, u.u.s.nanoseconds, u.u.s.fractional_nanoseconds,
		s1.u.s.nanoseconds_msb, s1.u.s.nanoseconds, s1.u.s.fractional_nanoseconds);

	ptp_double_to_scaled_ns(&s, d2);
	scaled_ns_to_u_scaled_ns(&u, &s);

	u_scaled_ns_to_scaled_ns(&s1, &u);
	scaled_ns_to_ptp_double(&d4, &s1);

	os_log(LOG_INFO, "%f %f %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx\n", d2, d4,
		s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds,
		u.u.s.nanoseconds_msb, u.u.s.nanoseconds, u.u.s.fractional_nanoseconds,
		s1.u.s.nanoseconds_msb, s1.u.s.nanoseconds, s1.u.s.fractional_nanoseconds);

	ptp_double_to_scaled_ns(&s, d3);
	scaled_ns_to_u_scaled_ns(&u, &s);

	u_scaled_ns_to_scaled_ns(&s1, &u);
	scaled_ns_to_ptp_double(&d4, &s1);

	os_log(LOG_INFO, "%f %f %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx %hx.%"PRIx64".%hx\n", d3, d4,
		s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds,
		u.u.s.nanoseconds_msb, u.u.s.nanoseconds, u.u.s.fractional_nanoseconds,
		s1.u.s.nanoseconds_msb, s1.u.s.nanoseconds, s1.u.s.fractional_nanoseconds);


	/* 802.1AS 6.3.3.1
	In scaled_ns -2.5 ns is expressed as: 0xFFFF FFFF FFFF FFFF FFFD 8000
	*/
	d1 = -2.5;
	ptp_double_to_scaled_ns(&s, d1);
	os_log(LOG_INFO, "%f ns = %hx.%"PRIx64".%hx scaled_ns (0xfffffffffffffffffffd8000)\n", d1, s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds);

	/* 802.1AS 6.3.3.2
	In u_scaled_ns 2.5 ns is expressed as: 0x0000 0000 0000 0000 0002 8000
	*/
	d1 = 2.5;
	ptp_double_to_u_scaled_ns(&u, d1);
	os_log(LOG_INFO, "%f ns = %hx.%"PRIx64".%hx u_scaled_ns (0x000000000000000000028000)\n", d1, u.u.s.nanoseconds_msb, u.u.s.nanoseconds, u.u.s.fractional_nanoseconds);
}

static void u_scaled_ns_unit_test(void)
{
	struct ptp_u_scaled_ns u1 = {
		.u.u_scaled_nanoseconds = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} /* Biggest value, smallest negative */
	};

	struct ptp_u_scaled_ns u2 = {
		.u.u_scaled_nanoseconds = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00} /* Smallest value */
	};

	struct ptp_u_scaled_ns u3 = {
		.u.u_scaled_nanoseconds = {0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* Biggest negative */
	};
#if 0
	struct ptp_u_scaled_ns u4 = {
		.u.u_scaled_nanoseconds = {0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} /* Biggest positive */
	};
#endif
	struct ptp_u_scaled_ns tmp, tmp1;

	minus_u_scaled_ns(&tmp, &u1);

	os_log(LOG_INFO, "- %hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n",
		u1.u.s.nanoseconds_msb, u1.u.s.nanoseconds, u1.u.s.fractional_nanoseconds,
		tmp.u.s.nanoseconds_msb, tmp.u.s.nanoseconds, tmp.u.s.fractional_nanoseconds);

	minus_u_scaled_ns(&tmp, &u3);

	os_log(LOG_INFO, "- %hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n",
		u3.u.s.nanoseconds_msb, u3.u.s.nanoseconds, u3.u.s.fractional_nanoseconds,
		tmp.u.s.nanoseconds_msb, tmp.u.s.nanoseconds, tmp.u.s.fractional_nanoseconds);

	minus_u_scaled_ns(&tmp, &u2);
	u_scaled_ns_add(&tmp1, &u2, &tmp);

	os_log(LOG_INFO, "%hx.%"PRIx64".%hx - %hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n",
		u2.u.s.nanoseconds_msb, u2.u.s.nanoseconds, u2.u.s.fractional_nanoseconds,
		u2.u.s.nanoseconds_msb, u2.u.s.nanoseconds, u2.u.s.fractional_nanoseconds,
		tmp1.u.s.nanoseconds_msb, tmp1.u.s.nanoseconds, tmp1.u.s.fractional_nanoseconds);

	u_scaled_ns_add(&tmp, &u3, &u3);

	os_log(LOG_INFO, "%hx.%"PRIx64".%hx + %hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n",
		u3.u.s.nanoseconds_msb, u3.u.s.nanoseconds, u3.u.s.fractional_nanoseconds,
		u3.u.s.nanoseconds_msb, u3.u.s.nanoseconds, u3.u.s.fractional_nanoseconds,
		tmp.u.s.nanoseconds_msb, tmp.u.s.nanoseconds, tmp.u.s.fractional_nanoseconds);

	u_scaled_ns_add(&tmp, &u1, &u2);

	os_log(LOG_INFO, "%hx.%"PRIx64".%hx + %hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n",
		u1.u.s.nanoseconds_msb, u1.u.s.nanoseconds, u1.u.s.fractional_nanoseconds,
		u2.u.s.nanoseconds_msb, u2.u.s.nanoseconds, u2.u.s.fractional_nanoseconds,
		tmp.u.s.nanoseconds_msb, tmp.u.s.nanoseconds, tmp.u.s.fractional_nanoseconds);
}


static void correction_to_scaled_ns_unit_test(void)
{
	#define NUM_CONV 6
	s64 c[NUM_CONV] = {htonll(0), htonll(0xffffffffffffffff), htonll(1), htonll((u64)-1), htonll(123456789123465789), htonll((u64)-123456789123456789)};
	struct ptp_scaled_ns s_res[NUM_CONV] = {
		{.u.s.nanoseconds_msb = 0x0, .u.s.nanoseconds = 0x0, .u.s.fractional_nanoseconds = 0x0}, // 0
		{.u.s.nanoseconds_msb = 0xffff, .u.s.nanoseconds = 0xffffffffffffffff, .u.s.fractional_nanoseconds = 0xffff}, // 0xffffffffffffffff
		{.u.s.nanoseconds_msb = 0x0, .u.s.nanoseconds = 0x0, .u.s.fractional_nanoseconds = 0x1}, // 1
		{.u.s.nanoseconds_msb = 0xffff, .u.s.nanoseconds = 0xffffffffffffffff, .u.s.fractional_nanoseconds = 0xffff}, // -1
		{.u.s.nanoseconds_msb = 0x0, .u.s.nanoseconds = 0x1b69b4bacd0, .u.s.fractional_nanoseconds = 0x823d}, // 123456789123465789
		{.u.s.nanoseconds_msb = 0xffff, .u.s.nanoseconds = 0xfffffe4964b4532f, .u.s.fractional_nanoseconds = 0xa0eb}, // -123456789123456789
	};
	struct ptp_scaled_ns s;
	int i;

	for (i = 0; i < NUM_CONV; i++) {
		pdu_correction_field_to_scaled_ns(&s, c[i]);
		os_log(LOG_INFO, "%"PRId64" = %hx.%"PRIx64".%hx\n", c[i], s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds);
		os_log(LOG_INFO, "%hx.%"PRIx64".%hx = %hx.%"PRIx64".%hx\n", s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds, s_res[i].u.s.nanoseconds_msb, s_res[i].u.s.nanoseconds, s_res[i].u.s.fractional_nanoseconds);
		scaled_ns_to_pdu_correction_field(&c[i], &s);
		os_log(LOG_INFO, "%hx.%"PRIx64".%hx = %"PRId64"\n", s.u.s.nanoseconds_msb, s.u.s.nanoseconds, s.u.s.fractional_nanoseconds, c[i]);
	}
}

static void misc_unit_test(void)
{
	int i;
	s64 t1[4] = {0, 123456789123456789, -123456789123456789, 123456789123456789};
	s64 t2[4] = {-1, 1, -1, -123456789123456789};

	os_log(LOG_INFO, "t1=%"PRId64" %"PRId64" %"PRId64" %"PRId64"\n", t1[0], t1[1], t1[2], t1[3]);
	os_log(LOG_INFO, "t2=%"PRId64" %"PRId64" %"PRId64" %"PRId64"\n", t2[0], t2[1], t2[2], t2[3]);

	for (i = 0; i < 4; i++) {
		os_log(LOG_INFO, "abs: t1=%"PRId64" t2=%"PRId64" -> r1=%u r2=%u\n", t1[i], t2[i], os_abs((u64)(t1[i] - t2[i])), os_abs((u64)(t2[i] - t1[i])));
		os_log(LOG_INFO, "llabs: t1=%"PRId64" t2=%"PRId64" -> r1=%llu r2=%llu\n", t1[i], t2[i], os_llabs((u64)(t1[i] - t2[i])), os_llabs((u64)(t2[i] - t1[i])));
	}

}
void ptp_time_ops_unit_test(void)
{
	u_scaled_ns_unit_test();
	ptp_double_unit_test();
	correction_to_scaled_ns_unit_test();
	misc_unit_test();
}
