/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <inttypes.h>

#include "time.h"
#include "ts_parser.h"

#define TS_PARSER_DEBUG		1

#define SYNC_BYTE	0x47

#define SYSTEM_CLOCK_FREQUENCY	27000000

#define NSECS_PER_USEC	1000
#define NSEC_PER_MSEC	1000000
#define USEC_PER_SEC	1000000

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct __attribute__ ((packed)) transport_header {
	u8 sync_byte;

	u8 pid_high:5;
        u8 priority:1;
        u8 payload_unit_start_indicator:1;
        u8 error_indication:1;

	u8 pid_low:8;

	u8 continuity_counter:4;
	u8 adaptation_field_control:2;
	u8 scrambling_control:2;
};

struct __attribute__ ((packed)) adaptation_header {
	u8 length;

	u8 extension_flag:1;
	u8 transport_private_data_flag:1;
	u8 splicing_point_flag:1;
	u8 opcr_flag:1;
	u8 pcr_flag:1;
	u8 elementary_stream_priority_indicator:1;
	u8 random_acess_indicator:1;
	u8 discontinuity_indicator:1;
};

struct __attribute__ ((packed)) pcr_header {
	u32 pcr_clock_reference_base_high;

	u8 program_clock_reference_extension_high:1;
	u8 reserved:6;
	u8 pcr_clock_reference_base_low:1;

	u8 program_clock_reference_extension_low;
};

static inline u64 pcr_to_ns(unsigned long long pcr)
{
	return (pcr * (NSECS_PER_SEC/1000000)) / (SYSTEM_CLOCK_FREQUENCY/1000000);
}

static inline u64 pcr_to_us(unsigned long long pcr)
{
	return (pcr * (USECS_PER_SEC/1000000)) / (SYSTEM_CLOCK_FREQUENCY/1000000);
}

static int adaptation_field(struct ts_parser *p, void *buf)
{
	struct adaptation_header *a_hdr = buf;

//	printf("%x %x\n", a_hdr->length, a_hdr->pcr_flag);

	if (!a_hdr->length)
		return 0;

	if (a_hdr->pcr_flag) {
		struct pcr_header *p_hdr = (struct pcr_header *)(a_hdr + 1);
		u64 pcr_base;
		u16 pcr_ext;
		u64 pcr;

		pcr_base = (((u64)ntohl(p_hdr->pcr_clock_reference_base_high)) << 1) | ((u64)p_hdr->pcr_clock_reference_base_low);
		pcr_ext = (((u16)p_hdr->program_clock_reference_extension_high) << 8) | ((u16)p_hdr->program_clock_reference_extension_low);
		pcr = pcr_base * 300 + pcr_ext;

		if (p->pcr_count == 1) {
			printf("pcr init\n");
			gettime_ns(&p->t0_ns);
			p->pcr0 = p->pcr[1];
		}

		p->pcr_count++;

		p->count[0] = p->count[1];
		p->pcr[0] = p->pcr[1];
		p->count[1] = p->byte_count;
		p->pcr[1] = pcr;

		if (p->pcr_count >= 2)
			p->transport_rate = ((p->count[1] - p->count[0]) * SYSTEM_CLOCK_FREQUENCY) / (p->pcr[1] - p->pcr[0]);
	}

	return 0;
}

static void print_pcr(unsigned long long byte, unsigned long long pcr, unsigned long long pcr_dt_ns)
{
#if TS_PARSER_DEBUG >= 2
	printf("packet %llu, byte %llu, pcr %llu, dt %llu\n", byte / PES_SIZE + 1, byte, pcr, pcr_dt_ns);
#endif
}

static void ts_parser_debug(struct ts_parser *p, unsigned long long dt_ns, unsigned long long byte)
{
#if TS_PARSER_DEBUG >= 3
	printf("%10llu %10llu %10llu %10llu %10llu %10llu %10u\n",
		p->count[0], byte, p->count[1],
		pcr_to_ns(p->pcr[0] - p->pcr0), dt_ns, pcr_to_ns(p->pcr[1] - p->pcr0), p->transport_rate);
#endif
}


static int transport_packet(struct ts_parser *p, void *buf)
{
	struct transport_header *t_hdr = buf;

	if (t_hdr->sync_byte != SYNC_BYTE)
		return -1;

//	printf("pid: %x\n", t_hdr->pid_low | (t_hdr->pid_high << 8));

	if ((t_hdr->adaptation_field_control == 0x2) || (t_hdr->adaptation_field_control == 0x3))
		adaptation_field(p, (void *)(t_hdr + 1));

	p->byte_count += PES_SIZE;

	return 0;
}

void ts_parser_init(struct ts_parser *p)
{
	memset(p, 0, sizeof(*p));
}

int ts_parser_is_pcr(void *buf, u64 *pcr)
{
	struct transport_header *t_hdr = buf;
	struct adaptation_header *a_hdr;
	struct pcr_header *p_hdr;
	u64 pcr_base;
	u16 pcr_ext;

	if (t_hdr->sync_byte != SYNC_BYTE)
		return 0;

//	printf("pid: %x\n", t_hdr->pid_low | (t_hdr->pid_high << 8));

	if ((t_hdr->adaptation_field_control != 0x2) && (t_hdr->adaptation_field_control != 0x3))
		return 0;

	a_hdr = (struct adaptation_header *)(t_hdr + 1);

//	printf("%x %x\n", a_hdr->length, a_hdr->pcr_flag);

	if (!a_hdr->length)
		return 0;

	if (!a_hdr->pcr_flag)
		return 0;

	p_hdr = (struct pcr_header *)(a_hdr + 1);

	pcr_base = (((u64)ntohl(p_hdr->pcr_clock_reference_base_high)) << 1) | ((u64)p_hdr->pcr_clock_reference_base_low);
	pcr_ext = (((u16)p_hdr->program_clock_reference_extension_high) << 8) | ((u16)p_hdr->program_clock_reference_extension_low);

	*pcr = pcr_base * 300 + pcr_ext;

	*pcr = pcr_to_ns(*pcr);

	return 1;
}

static int ts_parser_update_pcr(struct ts_parser *p, void *buf, unsigned int *len, unsigned long long byte)
{
	unsigned int offset = 0;
	int rc = 0;

	/* Parse buffer for PCR */
	while ((p->pcr_count < 2) || (byte > p->count[1])) {
		if (*len < PES_SIZE) {
			rc = -1;
			break;
		}

		transport_packet(p, buf + offset);

		offset += PES_SIZE;
		*len -= PES_SIZE;
	}

	*len = offset;

	return rc;
}

/* Returns relative time for a given stream byte (in nanoseconds, based on PCR's) */
static uint64_t ts_parser_get_dt_ns(struct ts_parser *p, unsigned long long byte)
{
	uint64_t dt_ns;

	if (p->pcr_count < 2) {
		/* Not enough data */
		dt_ns = 0;
	} else if (byte < p->count[0]) {
		/* Data before first PCR */
		dt_ns = 0;
	} else if (byte < p->count[1]) {
		dt_ns = pcr_to_ns(p->pcr[0] - p->pcr0) + ((byte - p->count[0]) * NSECS_PER_SEC) / p->transport_rate;

		if (byte == p->count[0])
			print_pcr(byte, p->pcr[0], dt_ns);

	} else if (byte == p->count[1]) {
		dt_ns = pcr_to_ns(p->pcr[1] - p->pcr0);

		print_pcr(byte, p->pcr[1], dt_ns);
	} else {
#if TS_PARSER_DEBUG >= 1
		printf("extrapolating pcr\n");
#endif

		dt_ns = pcr_to_ns(p->pcr[0] - p->pcr0) + ((byte - p->count[0]) * NSECS_PER_SEC) / p->transport_rate;
	}

	return dt_ns;
}

/* Returns system time remaining to the last known PCR (in us, modulo 2^32) */
static int ts_parser_time_to_last_pcr(struct ts_parser *p)
{
	uint64_t t_us;
	uint64_t dt_us;

	gettime_us(&t_us);
	dt_us = t_us - p->t0_ns/NSECS_PER_USEC;

	if (p->pcr_count >= 2)
		return pcr_to_us(p->pcr[1] - p->pcr0) - dt_us;
	else
		return 0;
}

/* Returns absolute time for a given stream byte (in ns, modulo 2^32) */
static unsigned int ts_parser_get_t_ns(struct ts_parser *p, unsigned long long byte)
{
	uint64_t dt_ns;
	uint64_t t_ns;
#if TS_PARSER_DEBUG >= 1
	uint64_t sys_t_ns;
	uint64_t sys_dt_ns;
#endif

	dt_ns = ts_parser_get_dt_ns(p, byte);
	t_ns = p->t0_ns + dt_ns;

	ts_parser_debug(p, dt_ns, byte);

#if TS_PARSER_DEBUG >= 1
	if (gettime_ns(&sys_t_ns) >= 0) {
		sys_dt_ns = sys_t_ns - p->t0_ns;

		if (t_ns < sys_t_ns) {
			printf("late: %10" PRId64 " %10" PRIu64 " %10" PRIu64 " %10" PRId64 "\n",
				dt_ns - sys_dt_ns, t_ns, sys_t_ns, t_ns - sys_t_ns);
		}
	}
#endif

	return (unsigned int)t_ns;
}

/* Returns absolute system time for a range of stream bytes (in an avb_event array) */
int ts_parser_timestamp_range(struct file_buffer *b, struct ts_parser *p, struct avb_event *event, unsigned int *event_len, unsigned int data_start, unsigned int data_len, unsigned int presentation_offset)
{
	unsigned int len;
	unsigned int event_n = 0;
	unsigned int offset = 0;
	unsigned int cur = data_start;
	int rc;

	while ((offset < data_len) && (event_n < *event_len)) {
		len = file_buffer_available_wrap(b, 1);

		ts_parser_update_pcr(p, file_buffer_buf(b, 1), &len, cur);

		file_buffer_read(b, 1, len);

		len = file_buffer_available_wrap(b, 1);

		rc = ts_parser_update_pcr(p, file_buffer_buf(b, 1), &len, cur);

		file_buffer_read(b, 1, len);

		if (rc < 0) {
			int timeout;
			struct timespec ts;

			/* If some progress was made, use it */
			if (offset)
				goto exit;

			if (!file_buffer_free(b, 1)) {
				printf("file buffer too small for ts parsing\n");
				offset = -1;
				goto exit;
			}

			if (p->pcr_count < 2) {
#if TS_PARSER_DEBUG >= 1
				printf("waiting for pcr\n");
#endif
				ts.tv_sec = 0;
				ts.tv_nsec = 10000;

				nanosleep(&ts, NULL);

				goto exit;
			}

			timeout = ts_parser_time_to_last_pcr(p);
			if (timeout > 20000) {
#if TS_PARSER_DEBUG >= 1
				printf("waiting for pcr %d\n", timeout);
#endif
				ts.tv_sec = (timeout / 2) / USEC_PER_SEC;
				ts.tv_nsec = ((timeout / 2) % USEC_PER_SEC) * NSECS_PER_USEC;
				nanosleep(&ts, NULL);

				goto exit;
			}
#if TS_PARSER_DEBUG >= 1
			printf("no pcr\n");
#endif
		}

		event[event_n].event_mask = AVTP_SYNC;
		event[event_n].ts = ts_parser_get_t_ns(p, cur) + presentation_offset;
		event[event_n].index = offset;

//		printf("%u %u %u %u %u %u %u\n", cur, offset, event_n, event[event_n].ts, data_start, data_len, *event_len);

		event_n++;
		offset += PES_SIZE;
		cur += PES_SIZE;
	}

exit:
	*event_len = event_n;

	return offset;
}
