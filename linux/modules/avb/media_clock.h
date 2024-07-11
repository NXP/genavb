/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MCLOCK_H_
#define _MCLOCK_H_

#include "genavb/types.h"

/* Media clock types */
typedef enum {
	REC, /* Recovery, associated to a HW audio clock domain  */
	GEN, /* Generation, associated to a HW audio clock domain */
	PTP  /* Virtual, not associated to a HW audio clock domain */
} mclock_t;

typedef enum {
	TS_EXTERNAL,		/* Timestamps are provided by user-space */
	TS_INTERNAL 		/* Timestamps are generated locally, in sync with PTP */
} mclock_ts_src_t;

typedef enum  {
	MCLOCK_CFG_FREQ,
	MCLOCK_CFG_TS_SRC,
} mclock_sconfig_cmd;

typedef enum  {
	MCLOCK_STOPPED = -1,
	MCLOCK_RUNNING = 0,
	MCLOCK_RUNNING_LOCKED = 1
} mclock_status;

struct mclock_sconfig {
	mclock_sconfig_cmd cmd;
	union {
		mclock_ts_src_t ts_src;
		struct {
			unsigned int p;
			unsigned int q;
		} ts_freq;
	};
};

struct mclock_gconfig {
	mclock_ts_src_t ts_src;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int array_size; /* in u32 timestamps */
	unsigned int mmap_size;
	unsigned int timer_period; /* ns */
};

struct mclock_start {
	u32 ts_0;
	u32 ts_1;
};

struct mclock_clean {
	unsigned int nb_clean;
	mclock_status status;
};

#ifdef __KERNEL__

#include <linux/wait.h>
#include "rational.h"

#define MCLOCK_TIMER_ACTIVE 	(1 << 0)

#define MCLOCK_DRIFT_PPM_MAX		250

#define FEC_TMODE_PULSE_HIGH		0xBC
#define FEC_TMODE_PULSE_LOW		0xB8
#define FEC_TMODE_TOGGLE		0x94
#define FEC_TMODE_CAPTURE_RISING	0x84


struct mclock_drv;

struct mclock_dev {
	mclock_t type;
	mclock_ts_src_t ts_src;
	struct list_head list;
	struct list_head mtimer_devices;
	unsigned int flags;
	int domain; //the audio domain ID
	struct eth_avb *eth;
	int id; //ISR id
	struct mclock_drv *drv;

	void *sh_mem;
	dma_addr_t sh_mem_dma;
	unsigned int sh_mem_size;
	unsigned int mmap_size;
	unsigned int num_ts;
	unsigned int *w_idx;
	unsigned int *ptp;
	unsigned int *count;

	int (*start)(struct mclock_dev *, struct mclock_start *);
	int (*stop)(struct mclock_dev *);
	int (*clean)(struct mclock_dev *, struct mclock_clean *);
	int (*reset)(struct mclock_dev *);
	int (*config)(struct mclock_dev *, struct mclock_sconfig *);
	int (*open)(struct mclock_dev *, int);
	void (*release)(struct mclock_dev *);

	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	struct rational ts_period;
	unsigned int clk_timer;
	unsigned int timer_period;
	unsigned int next_drift;
	unsigned int drift_period;
};

struct mclock_timer {
	unsigned int flags;
	struct mclock_dev *dev;
	int (*irq_func)(struct mclock_dev *, void *, unsigned int);
	unsigned int period;
	unsigned int count;
};

/*
 * External HW clock devices such as multiplier or divider
 * used to generate/recover audio clock from ENET
 * low frequency intermediate clock.
 */
struct mclock_ext_dev {
	struct device *dev;
	unsigned int flags;
	struct list_head list;
	int (*configure)(struct device *dev, unsigned int freq_p, unsigned int freq_q);
};

struct mtimer_dev;
struct mtimer_start;

void mclock_set_ts_freq(struct mclock_dev *dev, unsigned int ts_freq_p, unsigned int ts_freq_q);
int  mclock_drift_adapt(struct mclock_dev *dev, unsigned int clk_media);

void mclock_wake_up_init(struct mclock_dev *dev, unsigned int now_ns);
void mclock_wake_up_init_now(struct mclock_dev *dev, unsigned int now_ns);
int  mclock_wake_up_configure(struct mclock_dev *dev, unsigned int wake_freq_p, unsigned int wake_freq_q);
int  mclock_wake_up(struct mclock_dev *dev, unsigned int elapsed_ns);
void mclock_wake_up_thread(struct mclock_dev *dev, struct mtimer_dev **mtimer_dev_array, unsigned int *n);

int mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start);
void mclock_timer_stop(struct mtimer_dev *mtimer_dev);

int  mclock_open(struct mclock_dev *dev, int port);
void mclock_release(struct mclock_dev *dev);
int  mclock_start(struct mclock_dev *dev, struct mclock_start *start);
int  mclock_stop(struct mclock_dev *dev);
int  mclock_clean(struct mclock_dev *dev, struct mclock_clean *clean);
int  mclock_reset(struct mclock_dev *dev);
void mclock_gconfig(struct mclock_dev *dev, struct mclock_gconfig *cfg);
int  mclock_sconfig(struct mclock_dev *dev, struct mclock_sconfig *cfg);

static inline u32 ts_wa(u32 ts)
{
	if (ts < 0x8)
		return  0x8;
	else if (ts > 0xfffffff7)
		return 0xfffffff7;
	else
		return ts;
}

/*
 * Based on a single timer period adjustment at a time,
 * returns the period for drift adaptation.
 */
static inline u32 mclock_drift_period(struct mclock_dev *dev)
{
	return ((dev->timer_period / MCLOCK_DRIFT_PPM_MAX) * 1000000);
}

static inline unsigned int mclock_shmem_read(struct mclock_dev *dev, unsigned int idx)
{
	return *(unsigned int *)((char *) dev->sh_mem + ((idx) & (dev->num_ts - 1)) * sizeof(unsigned int));
}

static inline void mclock_shmem_write(struct mclock_dev *dev, unsigned int idx, unsigned int value)
{
	*(unsigned int *)((char *) dev->sh_mem + ((idx) & (dev->num_ts - 1)) * sizeof(unsigned int)) = value;
}

#define MCLOCK_FLAGS_FREE		(1 << 0)
#define MCLOCK_FLAGS_RUNNING		(1 << 1)
#define MCLOCK_FLAGS_REGISTERED		(1 << 2)
#define MCLOCK_FLAGS_WAKE_UP		(1 << 3)
#define MCLOCK_FLAGS_INIT		(1 << 4)
#define MCLOCK_FLAGS_RESET		(1 << 5)
#define MCLOCK_FLAGS_RUNNING_LOCKED	(1 << 6)

#endif /* __KERNEL__ */

#endif /* _MCLOCK_H_ */
