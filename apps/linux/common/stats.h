/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_STATS_H_
#define _COMMON_STATS_H_

struct stats {
	unsigned int log2_size;
	unsigned int current_count;

	int current_min;
	int current_max;
	long long current_mean;
	unsigned long long current_ms;

	/* Stats snapshot */
	int min;
	int max;
	int mean;
	unsigned long long ms;
	unsigned long long variance;

	/* absolute min/max (never reset) */
	int abs_min;
	int abs_max;

	void *priv;
	void (*func)(struct stats *s);
};

#define MAX_SLOTS 256

struct hist {
	unsigned int slots[MAX_SLOTS];
	unsigned int n_slots;
	unsigned int slot_size;
};

void stats_reset(struct stats *s);
void stats_print(struct stats *s);
void stats_init(struct stats *s, unsigned int log2_size, void *priv, void (*func)(struct stats *s));
void stats_update(struct stats *s, int val);
void stats_compute(struct stats *s);

int hist_init(struct hist *hist, unsigned int n_slots, unsigned slot_size);
void hist_update(struct hist *hist, unsigned int value);
void hist_reset(struct hist *hist);

void hist_print(struct hist *hist);
#endif /* _COMMON_STATS_H_ */
